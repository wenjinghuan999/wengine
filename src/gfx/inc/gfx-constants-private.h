#pragma once

#include "platform/inc/platform.inc"

#include "gfx/gfx-constants.h"

namespace wg {

namespace gfx_formats {

[[nodiscard]] inline Format FromVkFormat(vk::Format vk_format) {
    return static_cast<Format>(vk_format);
}

[[nodiscard]] inline vk::Format ToVkFormat(Format format) {
    return static_cast<vk::Format>(format);
}

} // namespace gfx_formats

[[nodiscard]] inline vk::QueueFlags GetRequiredQueueFlags(wg::gfx_queues::QueueId queue_id) {
    switch (queue_id) {
    case wg::gfx_queues::graphics:
    case wg::gfx_queues::present:
        return vk::QueueFlagBits::eGraphics;
    case wg::gfx_queues::transfer:
        return vk::QueueFlagBits::eTransfer;
    case wg::gfx_queues::compute:
        return vk::QueueFlagBits::eCompute;
    default:
        return {};
    }
}

struct QueueInfo {
    gfx_queues::QueueId queue_id{};
    uint32_t queue_family_index{};
    uint32_t queue_index_in_family{};
    vk::raii::Queue vk_queue{ nullptr };
    vk::raii::CommandPool vk_command_pool{ nullptr };
};

struct QueueInfoRef {
    gfx_queues::QueueId queue_id{};
    uint32_t queue_family_index{};
    uint32_t queue_index_in_family{};
    vk::raii::Device* vk_device{ nullptr };
    vk::Queue vk_queue{ nullptr };
    vk::CommandPool vk_command_pool{ nullptr };
};

} // namespace wg