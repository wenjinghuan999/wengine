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

void Gfx::Impl::createBufferResources(const std::shared_ptr<GfxBufferBase>& gfx_buffer, vk::BufferUsageFlags usage) {
    gfx_buffer->impl_->resources.reset();

    if (!gfx->logical_device_) {
        logger().error("Cannot create buffer resources because logical device is not available.");
        return;
    }
    gfx->waitDeviceIdle();

    if (!gfx_buffer->has_cpu_data()) {
        logger().error("Skip creating buffer resources because has no CPU data.");
        return;
    }

    auto resources = std::make_unique<BufferResources>();
    size_t data_size = gfx_buffer->data_size();

    auto buffer_create_info = vk::BufferCreateInfo{
        .size = data_size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };

    resources->buffer = gfx->logical_device_->impl_->vk_device.createBuffer(buffer_create_info);
    auto memory_requirements = resources->buffer.getMemoryRequirements();

    int memory_type_index = gfx->physical_device().impl_->findMemoryTypeIndex(memory_requirements,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    
    if (memory_type_index < 0) {
        logger().error("Cannot create vertex buffer resources because device cannot satisfy memory requirements.");
        resources->buffer = nullptr;
        return;
    }

    auto memory_allocate_info = vk::MemoryAllocateInfo{
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = static_cast<uint32_t>(memory_type_index)
    };

    resources->memory = gfx->logical_device_->impl_->vk_device.allocateMemory(memory_allocate_info);
    resources->buffer.bindMemory(*resources->memory, 0);

    void* mapped = resources->memory.mapMemory(0, data_size, {});
    std::memcpy(mapped, gfx_buffer->data(), data_size);
    resources->memory.unmapMemory();

    gfx_buffer->impl_->resources = gfx->logical_device_->impl_->buffer_resources.store(std::move(resources));

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
}


} // namespace wg