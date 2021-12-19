#pragma once

#include "platform/inc/platform.inc"
#include "gfx/shader.h"
#include "common/owned_resources.h"

namespace wg {

struct ShaderResources {
    vk::raii::ShaderModule shader_module{ nullptr };
};

struct Shader::Impl {
    std::vector<uint32_t> raw_data;
    OwnedResourceHandle<ShaderResources> resources;
    vk::PipelineShaderStageCreateInfo shader_stage_create_info{};
};

[[nodiscard]] inline vk::ShaderStageFlagBits GetShaderStageFlags(shader_stage::ShaderStage stage) {
    switch (stage)
    {
    case shader_stage::vert:
        return vk::ShaderStageFlagBits::eVertex;
    case shader_stage::frag:
        return vk::ShaderStageFlagBits::eFragment;
    default:
        return {};
    }
}

}