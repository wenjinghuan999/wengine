#pragma once

#include "platform/inc/platform.inc"
#include "gfx/gfx-pipeline.h"
#include "common/owned-resources.h"

namespace wg {

struct GfxPipelineResources {
    vk::raii::DescriptorSetLayout uniform_layout{nullptr};
    vk::raii::PipelineLayout pipeline_layout{nullptr};
    std::vector<vk::VertexInputBindingDescription> vertex_bindings;
    std::vector<vk::VertexInputAttributeDescription> vertex_attributes;
    vk::PipelineVertexInputStateCreateInfo vertex_input_create_info{};
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
    
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
    std::vector<vk::Buffer> vertex_buffers;
    std::vector<vk::DeviceSize> vertex_buffer_offsets;
    vk::Buffer index_buffer{nullptr};
    vk::DeviceSize index_buffer_offset{0};
    vk::IndexType index_type = vk::IndexType::eUint16;
    uint32_t vertex_count{0};
    uint32_t index_count{0};
    bool draw_indexed{false};
};

struct GfxPipeline::Impl {
    OwnedResourceHandle<GfxPipelineResources> resources;
};

}