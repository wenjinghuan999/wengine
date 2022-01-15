#pragma once

#include "platform/inc/platform.inc"
#include "gfx/shader.h"
#include "common/owned-resources.h"

namespace wg {

struct ShaderResources {
    vk::raii::ShaderModule shader_module{ nullptr };
};

struct Shader::Impl {
    std::vector<uint32_t> raw_data;
    OwnedResourceHandle <ShaderResources> resources;
    vk::PipelineShaderStageCreateInfo shader_stage_create_info{};
};

[[nodiscard]] inline vk::ShaderStageFlagBits GetShaderStageFlag(shader_stages::ShaderStage stage) {
    switch (stage) {
    case shader_stages::vert:
        return vk::ShaderStageFlagBits::eVertex;
    case shader_stages::frag:
        return vk::ShaderStageFlagBits::eFragment;
    default:
        return {};
    }
}

[[nodiscard]] inline vk::ShaderStageFlags GetShaderStageFlags(shader_stages::ShaderStages stages) {
    vk::ShaderStageFlags result;
    if (stages & shader_stages::vert) {
        result |= vk::ShaderStageFlagBits::eVertex;
    }
    if (stages & shader_stages::frag) {
        result |= vk::ShaderStageFlagBits::eFragment;
    }
    return result;
}

}