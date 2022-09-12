#pragma once

#include "platform/inc/platform.inc"

#include "gfx/draw-command.h"

#include "gfx/gfx-pipeline.h"
#include "gfx-pipeline-private.h"

#include <functional>

namespace wg {

namespace primitive_topologies {

inline vk::PrimitiveTopology ToVkPrimitiveTopology(PrimitiveTopology primitive_topology) {
    switch (primitive_topology) {
    case point_list:
        return vk::PrimitiveTopology::ePointList;
    case line_list:
        return vk::PrimitiveTopology::eLineList;
    case line_strip:
        return vk::PrimitiveTopology::eLineStrip;
    case triangle_list:
        return vk::PrimitiveTopology::eTriangleList;
    case triangle_strip:
        return vk::PrimitiveTopology::eTriangleStrip;
    case triangle_fan:
        return vk::PrimitiveTopology::eTriangleFan;
    case line_list_with_adjacency:
        return vk::PrimitiveTopology::eLineListWithAdjacency;
    case line_strip_with_adjacency:
        return vk::PrimitiveTopology::eLineStripWithAdjacency;
    case triangle_list_with_adjacency:
        return vk::PrimitiveTopology::eTriangleListWithAdjacency;
    case triangle_strip_with_adjacency:
        return vk::PrimitiveTopology::eTriangleStripWithAdjacency;
    default:
        return vk::PrimitiveTopology::ePatchList;
    }
}

} // namespace primitive_types

struct DrawCommand::Impl {
    std::vector<vk::Buffer> vertex_buffers;
    std::vector<vk::DeviceSize> vertex_buffer_offsets;
    vk::Buffer index_buffer{ nullptr };
    vk::DeviceSize index_buffer_offset{ 0 };
    vk::IndexType index_type = vk::IndexType::eUint16;
    uint32_t vertex_count{ 0 };
    uint32_t index_count{ 0 };
    bool draw_indexed{ false };

    std::vector<vk::VertexInputBindingDescription> vertex_bindings;
    std::vector<vk::VertexInputAttributeDescription> vertex_attributes;
    vk::PipelineVertexInputStateCreateInfo vertex_input_create_info;
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info;

    virtual void draw(vk::CommandBuffer& command_buffer) = 0;
};

struct SimpleDrawCommand::Impl : public DrawCommand::Impl {
    void draw(vk::CommandBuffer& command_buffer) override;
};

} // namespace wg