#include "platform/inc/platform.inc"

#include "gfx/gfx.h"
#include "common/config.h"
#include "common/logger.h"
#include "gfx-private.h"

namespace {

[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

namespace gfx_features {

const char* const FEATURE_NAMES[NUM_FEATURES_TOTAL] = {
    "window_surface",
    "separate_transfer",
    "sampler_anisotropy",
    "sampler_filter_cubic",
    "sampler_mirror_clamp_to_edge",
    "msaa",
    "sample_shading",
    "_must_enable_if_valid",
    "_debug_utils"
};

} // namespace gfx_features

[[nodiscard]] VulkanFeatures GetGlfwRequiredFeatures() {
    uint32_t glfw_extension_count = 0;
    const char** glfw_extension_names = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    VulkanFeatures glfw_features;
    glfw_features.instance_extensions = std::vector<const char*>(glfw_extension_names, glfw_extension_names + glfw_extension_count);

    static bool first_time = true;
    if (first_time) {
        logger().debug("GLFW requires {} extensions:", glfw_extension_count);
        for (const char* extension_name : glfw_features.instance_extensions) {
            logger().debug(" - {}", extension_name);
        }
        first_time = false;
    }

    return glfw_features;
}

VulkanFeatures VulkanFeatures::GetVulkanFeatures(wg::gfx_features::FeatureId feature) {
    switch (feature) {
    case wg::gfx_features::window_surface:
        return []() {
            auto vk_features = GetGlfwRequiredFeatures();
            vk_features.device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            vk_features.device_queues[wg::gfx_queues::graphics] = 1;
            vk_features.device_queues[wg::gfx_queues::present] = 1;
            return vk_features;
        }();
    case wg::gfx_features::separate_transfer:
        return []() {
            auto vk_features = VulkanFeatures();
            vk_features.device_queues[wg::gfx_queues::transfer] = 1;
            return vk_features;
        }();
    case wg::gfx_features::sampler_anisotropy:
        return {
            .device_features = {
                .samplerAnisotropy = true
            },
            .check_properties_and_features_func = [](
                const vk::PhysicalDeviceProperties& properties, const vk::PhysicalDeviceFeatures& features
            ) -> bool {
                return features.samplerAnisotropy;
            },
            .set_feature_func = [](vk::PhysicalDeviceFeatures& features) {
                features.samplerAnisotropy = true;
            }
        };
    case wg::gfx_features::sampler_filter_cubic:
        return {
            .device_extensions = { VK_EXT_FILTER_CUBIC_EXTENSION_NAME }
        };
    case wg::gfx_features::sampler_mirror_clamp_to_edge:
        return {
            .device_extensions = { VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME },
        };
    case wg::gfx_features::msaa:
        return {
            .check_properties_and_features_func = [](
                const vk::PhysicalDeviceProperties& properties, const vk::PhysicalDeviceFeatures& features
            ) -> bool {
                return properties.limits.framebufferColorSampleCounts > vk::SampleCountFlagBits::e1;
            },
        };
    case wg::gfx_features::sample_shading:
        return {
            .check_properties_and_features_func = [](
                const vk::PhysicalDeviceProperties& properties, const vk::PhysicalDeviceFeatures& features
            ) -> bool {
                return features.sampleRateShading;
            },
            .set_feature_func = [](vk::PhysicalDeviceFeatures& features) {
                features.sampleRateShading = true;
            }
        };
    case wg::gfx_features::_must_enable_if_valid:
        // VUID-VkDeviceCreateInfo-pProperties-04451
        // https://vulkan.lunarg.com/doc/view/1.2.198.1/mac/1.2-extensions/vkspec.html#VUID-VkDeviceCreateInfo-pProperties-04451
        return {
            .instance_extensions = { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME },
            .device_extensions = { VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME }
        };
#ifndef NDEBUG
    case wg::gfx_features::_debug_utils:
        return {
            .instance_layers = { "VK_LAYER_KHRONOS_validation" },
            .instance_extensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME }
        };
#endif
    default:
        return {};
    }
}

void GfxFeaturesManager::enableFeaturesByConfig(const PhysicalDevice& physical_device, GfxSetup& setup) {
    EngineConfig& config = EngineConfig::Get();

    if (config.get<bool>("gfx-separate-transfer")) {
        enableFeature(gfx_features::separate_transfer);
    }

    auto device_properties = physical_device.impl_->vk_physical_device.getProperties();
    float max_sampler_anisotropy = config.get<float>("gfx-max-sampler-anisotropy");
    if (max_sampler_anisotropy > 0.f) {
        enableFeature(gfx_features::sampler_anisotropy);
        setup.max_sampler_anisotropy = std::min(max_sampler_anisotropy, device_properties.limits.maxSamplerAnisotropy);
    }

    if (config.get<bool>("gfx-enable-sampler-filter-cubic")) {
        enableFeature(gfx_features::sampler_filter_cubic);
    }

    if (config.get<bool>("gfx-enable-sampler-mirror-clamp-to-edge")) {
        enableFeature(gfx_features::sampler_mirror_clamp_to_edge);
    }

    auto msaa_samples = config.get<int>("gfx-msaa-samples");
    if (msaa_samples > 1) {
        enableFeature(gfx_features::msaa);

        setup.msaa_samples = 1;
        for (int i = 1; i <= 64 && i <= msaa_samples; i <<= 1) {
            auto vk_samples = static_cast<vk::SampleCountFlagBits>(i);
            if (vk_samples & device_properties.limits.framebufferColorSampleCounts) {
                if (vk_samples & device_properties.limits.framebufferDepthSampleCounts) {
                    setup.msaa_samples = i;
                }
            }
        }

        if (setup.msaa_samples != msaa_samples) {
            logger().warn("Setting MSAA to {} failed, use MSAA = {} instead.", msaa_samples, setup.msaa_samples);
        }
    }

    if (config.get<bool>("gfx-enable-sample-shading")) {
        enableFeature(gfx_features::sample_shading);
    }
}

void Gfx::loadGlobalSetupFromConfig() {
}

} // namespace wg
