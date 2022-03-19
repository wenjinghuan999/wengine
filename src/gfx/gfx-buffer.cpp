#include "gfx/gfx-buffer.h"

#include "common/logger.h"
#include "gfx/gfx.h"
#include "gfx-private.h"
#include "gfx-buffer-private.h"

namespace {

[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

GfxMemoryBase::GfxMemoryBase(bool keep_cpu_data)
    : keep_cpu_data_(keep_cpu_data) {}
GfxMemoryBase::~GfxMemoryBase() = default;

GfxBufferBase::GfxBufferBase(bool keep_cpu_data)
    : GfxMemoryBase(keep_cpu_data), impl_(std::make_unique<Impl>()) {}
GfxBufferBase::~GfxBufferBase() = default;

GfxMemoryBase::Impl* GfxBufferBase::getImpl() {
    return impl_.get();
}

VertexBufferBase::VertexBufferBase(bool keep_cpu_data) : GfxBufferBase(keep_cpu_data) {}
VertexBufferBase::~VertexBufferBase() = default;

IndexBuffer::IndexBuffer(index_types::IndexType index_type, bool keep_cpu_data)
    : GfxBufferBase(keep_cpu_data), index_type_(index_type) {}
IndexBuffer::~IndexBuffer() = default;

UniformBufferBase::UniformBufferBase() : GfxBufferBase(true) {}
UniformBufferBase::~UniformBufferBase() = default;

std::shared_ptr<UniformBufferBase> UniformBufferBase::Create(uniform_attributes::UniformAttribute attribute) {
    switch (attribute) {
    case uniform_attributes::scene:
        return UniformBuffer<SceneUniform>::Create();
    case uniform_attributes::camera:
        return UniformBuffer<CameraUniform>::Create();
    case uniform_attributes::material:
        return UniformBuffer<MaterialUniform>::Create();
    case uniform_attributes::model:
        return UniformBuffer<ModelUniform>::Create();
    default:
        return {};
    }
}

void Gfx::Impl::singleTimeCommand(
    const QueueInfoRef& queue, const std::function<void(vk::CommandBuffer&)>& func,
    std::vector<vk::Semaphore> wait_semaphores, std::vector<vk::PipelineStageFlags> wait_stages, std::vector<vk::Semaphore> signal_semaphores
) {
    auto command_buffer_allocate_info = vk::CommandBufferAllocateInfo{
        .commandPool = queue.vk_command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    // Do not use vk::raii::CommandBuffer because there might be compiling errors on MSVC
    // Since we are allocating from the pool, we can destruct command buffers together.
    auto command_buffers =
        (*gfx->logical_device_->impl_->vk_device).allocateCommandBuffers(command_buffer_allocate_info);
    auto& command_buffer = command_buffers[0];
    command_buffer.begin(
        vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        }
    );

    func(command_buffer);

    command_buffer.end();

    auto submit_info = vk::SubmitInfo{}
        .setCommandBuffers(command_buffers)
        .setWaitSemaphores(wait_semaphores)
        .setWaitDstStageMask(wait_stages)
        .setSignalSemaphores(signal_semaphores);
    queue.vk_queue.submit({ submit_info });
    queue.vk_queue.waitIdle();

    (*gfx->logical_device_->impl_->vk_device).freeCommandBuffers(queue.vk_command_pool, command_buffers);
}

bool Gfx::Impl::createGfxMemory(
    vk::MemoryRequirements memory_requirements, vk::MemoryPropertyFlags memory_properties,
    GfxMemoryResources& out_resources
) {
    int memory_type_index = gfx->physical_device().impl_->findMemoryTypeIndex(memory_requirements, memory_properties);

    if (memory_type_index < 0) {
        logger().error("Cannot create memory resources because device cannot satisfy memory requirements.");
        return false;
    }

    auto memory_allocate_info = vk::MemoryAllocateInfo{
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = static_cast<uint32_t>(memory_type_index)
    };

    out_resources.memory = gfx->logical_device_->impl_->vk_device.allocateMemory(memory_allocate_info);
    out_resources.memory_properties = memory_properties;
    return true;
}

void Gfx::Impl::createBuffer(
    vk::DeviceSize data_size, vk::BufferUsageFlags usage,
    vk::SharingMode sharing_mode, const std::vector<uint32_t>& queue_family_indices,
    GfxBufferResources& out_resources
) {
    auto buffer_create_info = vk::BufferCreateInfo{
        .size = data_size,
        .usage = usage,
        .sharingMode = sharing_mode,
    }
        .setQueueFamilyIndices(queue_family_indices);

    out_resources.buffer = gfx->logical_device_->impl_->vk_device.createBuffer(buffer_create_info);
    out_resources.sharing_mode = sharing_mode;
}

void Gfx::Impl::copyBuffer(const QueueInfoRef& transfer_queue, vk::Buffer src, vk::Buffer dst, vk::DeviceSize size) {
    singleTimeCommand(
        transfer_queue,
        [&src, &dst, size](vk::CommandBuffer& command_buffer) {
            command_buffer.copyBuffer(src, dst, { { .srcOffset = 0, .dstOffset = 0, .size = size } });
        }
    );
}

vk::SharingMode Gfx::Impl::getTransferQueue(QueueInfoRef& out_transfer_queue) const {
    bool transfer_queue_different_family = false;
    if (gfx->features_manager().feature_enabled(gfx_features::separate_transfer)) {
        // Use 0 for now
        out_transfer_queue = gfx->logical_device_->impl_->queue_references[gfx_queues::transfer][0];
        uint32_t graphics_family_index = gfx->logical_device_->impl_->queue_references[gfx_queues::graphics][0].queue_family_index;
        transfer_queue_different_family = out_transfer_queue.queue_family_index != graphics_family_index;
    } else {
        // Use 0 for now
        out_transfer_queue = gfx->logical_device_->impl_->queue_references[gfx_queues::graphics][0];
    }

    return transfer_queue_different_family ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
}

void Gfx::Impl::createBufferResources(
    const std::shared_ptr<GfxBufferBase>& gfx_buffer,
    vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memory_properties
) {
    createReferenceBufferResources(gfx_buffer, gfx_buffer, usage, memory_properties);
}

void Gfx::Impl::createReferenceBufferResources(
    const std::shared_ptr<GfxBufferBase>& cpu_buffer,
    const std::shared_ptr<GfxBufferBase>& gpu_buffer,
    vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memory_properties
) {
    gpu_buffer->impl_->memory_resources.reset();
    gpu_buffer->impl_->resources.reset();
    gpu_buffer->has_gpu_data_ = false;

    if (!gfx->logical_device_) {
        logger().error("Cannot create buffer resources because logical device is not available.");
        return;
    }
    gfx->waitDeviceIdle();

    auto resources = std::make_unique<GfxBufferResources>();

    QueueInfoRef transfer_queue;
    resources->cpu_data_size = cpu_buffer->data_size();
    resources->sharing_mode = getTransferQueue(transfer_queue);

    uint32_t graphics_family_index = gfx->logical_device_->impl_->queue_references[gfx_queues::graphics][0].queue_family_index;
    std::vector<uint32_t> queue_family_indices = { graphics_family_index };
    if (graphics_family_index != transfer_queue.queue_family_index) {
        queue_family_indices.push_back(transfer_queue.queue_family_index);
    }

    createBuffer(
        resources->cpu_data_size,
        vk::BufferUsageFlagBits::eTransferDst | usage,
        resources->sharing_mode, queue_family_indices,
        *resources
    );

    auto memory_resources = std::make_unique<GfxMemoryResources>();
    if (!createGfxMemory(resources->buffer.getMemoryRequirements(), memory_properties, *memory_resources)) {
        return;
    }

    resources->buffer.bindMemory(*memory_resources->memory, 0);
    gpu_buffer->impl_->memory_resources = gfx->logical_device_->impl_->memory_resources.store(std::move(memory_resources));
    gpu_buffer->impl_->resources = gfx->logical_device_->impl_->buffer_resources.store(std::move(resources));
}

void Gfx::commitBuffer(const std::shared_ptr<GfxBufferBase>& gfx_buffer, bool hint_use_stage_buffer) {
    commitReferenceBuffer(gfx_buffer, gfx_buffer, hint_use_stage_buffer);
}

void Gfx::commitReferenceBuffer(
    const std::shared_ptr<GfxBufferBase>& cpu_buffer,
    const std::shared_ptr<GfxBufferBase>& gpu_buffer, bool hint_use_stage_buffer
) {
    if (!cpu_buffer->has_cpu_data()) {
        logger().error("Skip creating buffer resources because has no CPU data.");
        return;
    }
    waitDeviceIdle();

    auto* memory_resources = gpu_buffer->impl_->memory_resources.data();
    auto* resources = gpu_buffer->impl_->resources.data();
    if (!memory_resources || !resources) {
        logger().error("Cannot commit buffer because resources are not available.");
        return;
    }

    if (cpu_buffer->data_size() != resources->cpu_data_size) {
        logger().error("Cannot commit buffer because cpu data size differs from gpu resources.");
        return;
    }

    QueueInfoRef transfer_queue;
    impl_->getTransferQueue(transfer_queue);

    const size_t data_size = cpu_buffer->data_size();
    bool use_stage_buffer = !(memory_resources->memory_properties & vk::MemoryPropertyFlagBits::eHostVisible);
    if (hint_use_stage_buffer) {
        use_stage_buffer = true;
    }

    GfxMemoryResources staging_memory_resources;
    GfxBufferResources staging_resources;
    vk::raii::DeviceMemory* memory = &memory_resources->memory;
    if (use_stage_buffer) {
        // Allocate stage buffer.
        impl_->createBuffer(
            data_size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive, { transfer_queue.queue_family_index },
            staging_resources
        );
        if (!impl_->createGfxMemory(
            staging_resources.buffer.getMemoryRequirements(),
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            staging_memory_resources
        )) {
            return;
        }
        staging_resources.buffer.bindMemory(*staging_memory_resources.memory, 0);
        memory = &staging_memory_resources.memory;
    }

    // Copy data to target buffer or stage buffer.
    void* mapped = memory->mapMemory(0, data_size, {});
    std::memcpy(mapped, cpu_buffer->data(), data_size);
    memory->unmapMemory();

    if (use_stage_buffer) {
        // Copy content.
        impl_->copyBuffer(transfer_queue, *staging_resources.buffer, *resources->buffer, data_size);
    }

    gpu_buffer->has_gpu_data_ = true;
    if (!cpu_buffer->keep_cpu_data_) {
        cpu_buffer->clearCpuData();
    }
}

void Gfx::createVertexBufferResources(const std::shared_ptr<VertexBufferBase>& vertex_buffer) {
    impl_->createBufferResources(
        vertex_buffer,
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    commitBuffer(vertex_buffer, true);
}

void Gfx::createIndexBufferResources(const std::shared_ptr<IndexBuffer>& index_buffer) {
    impl_->createBufferResources(
        index_buffer,
        vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    commitBuffer(index_buffer, true);
}

void Gfx::createUniformBufferResources(const std::shared_ptr<UniformBufferBase>& uniform_buffer) {
    impl_->createBufferResources(
        uniform_buffer,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    if (uniform_buffer->has_cpu_data()) {
        commitBuffer(uniform_buffer, false);
    }
}

} // namespace wg