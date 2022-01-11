#include "gfx/gfx.h"
#include "gfx/gfx-buffer.h"
#include "common/logger.h"
#include "gfx-private.h"
#include "gfx-buffer-private.h"

namespace {
    
[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

GfxBufferBase::GfxBufferBase(bool keep_cpu_data) : impl_(std::make_unique<GfxBufferBase::Impl>()) {}
GfxBufferBase::~GfxBufferBase() {}

VertexBufferBase::VertexBufferBase(bool keep_cpu_data) : GfxBufferBase(keep_cpu_data) {}
VertexBufferBase::~VertexBufferBase() {}

IndexBuffer::IndexBuffer(index_types::IndexType index_type, bool keep_cpu_data)
    : GfxBufferBase(keep_cpu_data), index_type_(index_type) {}
IndexBuffer::~IndexBuffer() {}

UniformBufferBase::UniformBufferBase() : GfxBufferBase(true) {}
UniformBufferBase::~UniformBufferBase() {}

bool Gfx::Impl::createBuffer(vk::DeviceSize data_size, 
    vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memeory_properties, vk::SharingMode sharing_mode,
    BufferResources& out_resources) {
    auto buffer_create_info = vk::BufferCreateInfo{
        .size = data_size,
        .usage = usage,
        .sharingMode = sharing_mode
    };

    out_resources.buffer = gfx->logical_device_->impl_->vk_device.createBuffer(buffer_create_info);
    auto memory_requirements = out_resources.buffer.getMemoryRequirements();

    int memory_type_index = gfx->physical_device().impl_->findMemoryTypeIndex(memory_requirements, memeory_properties);
    
    if (memory_type_index < 0) {
        logger().error("Cannot create buffer resources because device cannot satisfy memory requirements.");
        out_resources.buffer = nullptr;
        return false;
    }

    auto memory_allocate_info = vk::MemoryAllocateInfo{
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = static_cast<uint32_t>(memory_type_index)
    };

    out_resources.memory = gfx->logical_device_->impl_->vk_device.allocateMemory(memory_allocate_info);
    out_resources.buffer.bindMemory(*out_resources.memory, 0);

    out_resources.memory_properties = memeory_properties;
    out_resources.sharing_mode = sharing_mode;
    return true;
}

void Gfx::Impl::copyBuffer(const QueueInfoRef& transfer_queue_info, vk::Buffer src, vk::Buffer dst, vk::DeviceSize size) {
    auto command_buffer_allocate_info = vk::CommandBufferAllocateInfo{
        .commandPool = transfer_queue_info.vk_command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1U
    };
    // Do not use vk::raii::CommandBuffer because there might be compile errors on MSVC
    // Since we are allocating from the pool, we can destruct command buffers together.
    auto command_buffers = 
        (*gfx->logical_device_->impl_->vk_device).allocateCommandBuffers(command_buffer_allocate_info);
    auto& command_buffer = command_buffers[0];

    command_buffer.begin(vk::CommandBufferBeginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    });
    command_buffer.copyBuffer(src, dst, {{ .srcOffset = 0, .dstOffset = 0, .size = size }});
    command_buffer.end();

    auto submit_info = vk::SubmitInfo{}
        .setCommandBuffers(command_buffers);
    transfer_queue_info.vk_queue.submit({ submit_info });
    transfer_queue_info.vk_queue.waitIdle();

    (*gfx->logical_device_->impl_->vk_device).freeCommandBuffers(transfer_queue_info.vk_command_pool, command_buffers);
}

vk::SharingMode Gfx::Impl::getTransferQueueInfo(QueueInfoRef& out_transfer_queue_info) const {
    bool transfer_queue_different_family = false;
    auto& gfx_features_manager = GfxFeaturesManager::Get();
    if (gfx_features_manager.feature_enabled(gfx_features::separate_transfer)) {
        // Use 0 for now
        out_transfer_queue_info = gfx->logical_device_->impl_->queue_references[gfx_queues::transfer][0];
        uint32_t graphics_family_index = gfx->logical_device_->impl_->queue_references[gfx_queues::graphics][0].queue_family_index;
        transfer_queue_different_family = out_transfer_queue_info.queue_family_index != graphics_family_index;
    } else {
        // Use 0 for now
        out_transfer_queue_info = gfx->logical_device_->impl_->queue_references[gfx_queues::graphics][0];
    }
    
    return transfer_queue_different_family ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
}

void Gfx::Impl::createBufferResources(const std::shared_ptr<GfxBufferBase>& gfx_buffer, vk::BufferUsageFlags usage) {
    gfx_buffer->impl_->resources.reset();

    if (!gfx->logical_device_) {
        logger().error("Cannot create buffer resources because logical device is not available.");
        return;
    }
    gfx->waitDeviceIdle();

    auto resources = std::make_unique<BufferResources>();
    const size_t data_size = gfx_buffer->data_size();

    gfx_buffer->impl_->resources = gfx->logical_device_->impl_->buffer_resources.store(std::move(resources));

    QueueInfoRef transfer_queue_info;
    vk::SharingMode sharing_mode = getTransferQueueInfo(transfer_queue_info);

    // Create buffer.
    createBuffer(data_size, 
        vk::BufferUsageFlagBits::eTransferDst | usage,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        sharing_mode, *gfx_buffer->impl_->resources.get());
}

void Gfx::commitBuffer(const std::shared_ptr<GfxBufferBase>& gfx_buffer, bool hint_use_stage_buffer) {

    if (!gfx_buffer->has_cpu_data()) {
        logger().error("Skip creating buffer resources because has no CPU data.");
        return;
    }
    waitDeviceIdle();

    auto* resources = gfx_buffer->impl_->resources.get();
    if (!resources) {
        logger().error("Cannot commit buffer because buffer resources are not available.");
        return;
    }

    QueueInfoRef transfer_queue_info;
    vk::SharingMode sharing_mode = impl_->getTransferQueueInfo(transfer_queue_info);
    
    const size_t data_size = gfx_buffer->data_size();
    bool use_stage_buffer = !(resources->memory_properties & vk::MemoryPropertyFlagBits::eHostVisible);
    if (hint_use_stage_buffer) {
        use_stage_buffer = true;
    }
    
    BufferResources staging_resources;
    vk::raii::DeviceMemory* memory = &resources->memory;
    if (use_stage_buffer) {
        // Allocate stage buffer.
        impl_->createBuffer(data_size, 
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            sharing_mode, staging_resources);
        memory = &staging_resources.memory;
    }

    // Copy data to target buffer or stage buffer.
    void* mapped = memory->mapMemory(0, data_size, {});
    std::memcpy(mapped, gfx_buffer->data(), data_size);
    memory->unmapMemory();

    if (use_stage_buffer) {
        // Copy content.
        impl_->copyBuffer(transfer_queue_info, *staging_resources.buffer, *resources->buffer, data_size);
    }

    gfx_buffer->has_gpu_data_ = true;
    if (!gfx_buffer->keep_cpu_data_) {
        gfx_buffer->clearCpuData();
    }
}

void Gfx::createVertexBufferResources(const std::shared_ptr<VertexBufferBase>& vertex_buffer) {
    impl_->createBufferResources(
        vertex_buffer, 
        vk::BufferUsageFlagBits::eVertexBuffer
    );
    commitBuffer(vertex_buffer);
}

void Gfx::createIndexBufferResources(const std::shared_ptr<IndexBuffer>& index_buffer) {
    impl_->createBufferResources(
        index_buffer, 
        vk::BufferUsageFlagBits::eIndexBuffer
    );
    commitBuffer(index_buffer);
}

void Gfx::createUniformBufferResources(const std::shared_ptr<UniformBufferBase>& uniform_buffer) {
    impl_->createBufferResources(
        uniform_buffer, 
        vk::BufferUsageFlagBits::eUniformBuffer
    );
    if (uniform_buffer->has_cpu_data()) {
        commitBuffer(uniform_buffer);
    }
}

} // namespace wg