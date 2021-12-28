#include <fstream>

#include "gfx/gfx.h"
#include "gfx/vertex-buffer.h"
#include "common/logger.h"
#include "gfx-private.h"
#include "vertex-buffer-private.h"

namespace {
    
[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

VertexBufferBase::VertexBufferBase(bool keep_cpu_data) : keep_cpu_data_(keep_cpu_data), impl_(std::make_unique<Impl>()) {}

VertexBufferBase::~VertexBufferBase() {}

void Gfx::createVertexBufferResources(const std::shared_ptr<VertexBufferBase>& vertex_buffer) {
    vertex_buffer->impl_->resources.reset();

    if (!logical_device_) {
        logger().error("Cannot create shader resources because logical device is not available.");
        return;
    }
    waitDeviceIdle();

    if (!vertex_buffer->has_cpu_data()) {
        logger().error("Skip creating vertex buffer resources because has no CPU data.");
        return;
    }

    auto resources = std::make_unique<VertexBufferResources>();
    size_t data_size = vertex_buffer->data_size();

    auto buffer_create_info = vk::BufferCreateInfo{
        .size = data_size,
        .usage = vk::BufferUsageFlagBits::eVertexBuffer,
        .sharingMode = vk::SharingMode::eExclusive
    };

    resources->buffer = logical_device_->impl_->vk_device.createBuffer(buffer_create_info);
    auto memory_requirements = resources->buffer.getMemoryRequirements();

    int memory_type_index = physical_device().impl_->findMemoryTypeIndex(memory_requirements,
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

    resources->memory = logical_device_->impl_->vk_device.allocateMemory(memory_allocate_info);
    resources->buffer.bindMemory(*resources->memory, 0);

    void* mapped = resources->memory.mapMemory(0, data_size, {});
    std::memcpy(mapped, vertex_buffer->data(), data_size);
    resources->memory.unmapMemory();

    vertex_buffer->impl_->resources = logical_device_->impl_->vertex_buffer_resources.store(std::move(resources));

    vertex_buffer->has_gpu_data_ = true;
    if (!vertex_buffer->keep_cpu_data_) {
        vertex_buffer->clearCpuData();
    }
}


} // namespace wg