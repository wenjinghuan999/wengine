#include "gfx/gfx.h"

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <algorithm>
#include <map>
#include <optional>
#include <vector>

#include "engine/engine.h"
#include "platform/platform.h"

namespace {

    auto logger = spdlog::stdout_color_mt("gfx");
    const std::vector<const char*> VALIDATION_LAYER_NAMES = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> DEBUG_UTILS_EXTENSION_NAMES = {
        "VK_EXT_debug_utils"
    };

    bool CheckLayersAvailable(const std::vector<vk::LayerProperties>& available_layers, const std::vector<const char*>& layer_names) {
        for (auto&& layer_name : layer_names) {
            auto found = std::find_if(available_layers.begin(), available_layers.end(), 
                [&layer_name](const vk::LayerProperties& layer) {
                    return std::string_view(layer.layerName) == std::string_view(layer_name);
                });
            if (available_layers.end() == found) {
                return false;
            }
        }
        return true;
    }

    bool CheckExtensionsAvailable(const std::vector<vk::ExtensionProperties>& available_extensions, const std::vector<const char*>& extension_names) {
        for (auto&& extensionName : extension_names) {
            auto found = std::find_if(available_extensions.begin(), available_extensions.end(), 
                [&extensionName](const vk::ExtensionProperties& extension) {
                    return std::string_view(extension.extensionName) == std::string_view(extensionName);
                });
            if (available_extensions.end() == found) {
                return false;
            }
        }
        return true;
    }

    bool AddDebugUtils(
        std::vector<const char*>& enabled_layer_names, std::vector<const char*>& enabled_extension_names,
        const std::vector<vk::LayerProperties>& available_layers, const std::vector<vk::ExtensionProperties>& available_extensions
        ) {
#ifdef NDEBUG
        return false;
#else
        if (CheckLayersAvailable(available_layers, VALIDATION_LAYER_NAMES)) {
            enabled_layer_names.insert(enabled_layer_names.end(), VALIDATION_LAYER_NAMES.begin(), VALIDATION_LAYER_NAMES.end());
        } else {
            logger->warn("Debug validation layers unavailable.");
        }
        if (CheckExtensionsAvailable(available_extensions, DEBUG_UTILS_EXTENSION_NAMES)) {
            enabled_extension_names.insert(enabled_extension_names.end(), DEBUG_UTILS_EXTENSION_NAMES.begin(), DEBUG_UTILS_EXTENSION_NAMES.end());
            return true;
        } else {
            logger->warn("Debug utils extentions unavailable.");
            return false;
        }
#endif
    }

    auto VulkanLogger = spdlog::stdout_color_mt("vulkan");

    VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageHandler(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        VkDebugUtilsMessengerCallbackDataEXT const * pCallbackData, void * /*pUserData*/ ) {
        
        spdlog::level::level_enum level;
        auto severity = static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity);
        if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
            level = spdlog::level::err;
        } else if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
            level = spdlog::level::warn;
        } else if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo) {
            level = spdlog::level::info;
        } else {
            level = spdlog::level::debug;
        }

        std::string type_string;
        type_string.reserve(3);
        auto types = static_cast<vk::DebugUtilsMessageTypeFlagBitsEXT>(messageTypes);
        if (types & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral) {
            type_string += 'G';
        }
        if (types & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation) {
            type_string += 'V';
        }
        if (types & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance) {
            type_string += 'P';
        }

        VulkanLogger->log(level, "[{}][{}] {}", pCallbackData->pMessageIdName, type_string, pCallbackData->pMessage);

        return false;
    }
                                                    
    std::unique_ptr<vk::raii::DebugUtilsMessengerEXT> CreateDebugMessenger(const vk::raii::Instance& instance) {
        vk::DebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info{
            .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | 
                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            .messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                               vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                               vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
            .pfnUserCallback = &DebugMessageHandler
        };
        return std::make_unique<vk::raii::DebugUtilsMessengerEXT>(instance, debug_utils_messenger_create_info, nullptr);
    }

    void AddGlfwUtils(
        std::vector<const char*>& enabled_layer_names, std::vector<const char*>& enabled_extension_names,
        const std::vector<vk::LayerProperties>& available_layers, const std::vector<vk::ExtensionProperties>& available_extensions
    ) {
        uint32_t glfw_extension_count = 0;
        const char** glfw_extension_names = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        enabled_extension_names.insert(enabled_extension_names.end(), glfw_extension_names, glfw_extension_names + glfw_extension_count);
    }
}
namespace wg {

struct Gfx::Impl {
    std::unique_ptr<vk::raii::Context> context;
    std::unique_ptr<vk::raii::Instance> instance;
    std::unique_ptr<vk::raii::DebugUtilsMessengerEXT> debug_messenger;
};

Gfx::Gfx(const App& app)
    : impl_(std::make_unique<Gfx::Impl>()) {
    
    logger->info("Initializing gfx.");

    impl_->context = std::make_unique<vk::raii::Context>();

    auto instance_version = impl_->context->enumerateInstanceVersion();
    logger->info("Instance version: {}.{}.{}", VK_API_VERSION_MAJOR(instance_version), 
        VK_API_VERSION_MINOR(instance_version), VK_API_VERSION_PATCH(instance_version));
    
    std::vector<vk::ExtensionProperties> available_extensions = impl_->context->enumerateInstanceExtensionProperties();
    logger->info("{} extensions available.", available_extensions.size());
    for (auto&& extension : available_extensions) {
        logger->debug(" - {} ({}.{}.{})", 
            extension.extensionName, VK_API_VERSION_MAJOR(extension.specVersion), 
            VK_API_VERSION_MINOR(extension.specVersion), VK_API_VERSION_PATCH(extension.specVersion));
    }
    
    std::vector<vk::LayerProperties> available_layers = impl_->context->enumerateInstanceLayerProperties();
    logger->info("{} layers available.", available_layers.size());
    for (auto&& layer : available_layers) {
        logger->debug(" - {} ({}.{}.{}, {}.{}.{}) {}", 
            layer.layerName, VK_API_VERSION_MAJOR(layer.specVersion), 
            VK_API_VERSION_MINOR(layer.specVersion), VK_API_VERSION_PATCH(layer.specVersion), 
            VK_API_VERSION_MAJOR(layer.implementationVersion), VK_API_VERSION_MINOR(layer.implementationVersion), 
            VK_API_VERSION_PATCH(layer.implementationVersion), layer.description);
    }

    const Engine& engine = Engine::get();

    vk::ApplicationInfo application_info{
        .pApplicationName   = app.name().c_str(),
        .applicationVersion = VK_MAKE_VERSION(app.major_version(), app.minor_version(), app.patch_version()),
        .pEngineName        = engine.name().c_str(),
        .engineVersion      = VK_MAKE_VERSION(engine.major_version(), engine.minor_version(), engine.patch_version()),
        .apiVersion         = VK_API_VERSION_1_0
    };

    std::vector<const char*> enabled_layer_names;
    std::vector<const char*> enabled_extension_names;

    // Adding debug layers and extensions
    const bool debug_utils_added = AddDebugUtils(enabled_layer_names, enabled_extension_names, available_layers, available_extensions);

    // Adding GLFW required extensions
    AddGlfwUtils(enabled_layer_names, enabled_extension_names, available_layers, available_extensions);

    logger->info("Enabled layers:");
    for (auto&& layer_name : enabled_layer_names) {
        logger->info(" - {}", layer_name);
    }

    logger->info("Enabled extensions:");
    for (auto&& extension_name : enabled_extension_names) {
        logger->info(" - {}", extension_name);
    }

    vk::InstanceCreateInfo instance_create_info{
        .pApplicationInfo = &application_info
    };
    instance_create_info.setPEnabledLayerNames(enabled_layer_names);
    instance_create_info.setPEnabledExtensionNames(enabled_extension_names);

    impl_->instance = std::make_unique<vk::raii::Instance>(*impl_->context, instance_create_info);

    if (debug_utils_added) {
        impl_->debug_messenger = CreateDebugMessenger(*impl_->instance.get());
    }

    // Devices
    updatePhysicalDevices();
}

Gfx::~Gfx() {
    logger->info("Destroying gfx.");
    logical_device_.reset();
    physical_devices_.clear();
}

bool Gfx::valid() const {
    return physical_device_valid();
}
struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
};

namespace {

QueueFamilyIndices FindQueueFamilies(const vk::raii::PhysicalDevice& vk_physical_device) {
    auto queue_families = vk_physical_device.getQueueFamilyProperties();
    QueueFamilyIndices indices;
    for (size_t i = 0; i < queue_families.size(); ++i) {
        auto& queue_family = queue_families[i];
        if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphics_family = static_cast<int>(i);
        }
    }
    return indices;
}

}

struct PhysicalDevice::Impl {
    vk::raii::PhysicalDevice vk_physical_device;
    QueueFamilyIndices queue_families;
    
    Impl(vk::raii::PhysicalDevice vk_physical_device) 
        : vk_physical_device(std::move(vk_physical_device)) {
        queue_families = FindQueueFamilies(this->vk_physical_device);
    }
};

PhysicalDevice::PhysicalDevice(const std::string& name)
    : name_(name), impl_() {}

void Gfx::updatePhysicalDevices() {
    physical_devices_.clear();

    auto vk_physical_devices = vk::raii::PhysicalDevices(*impl_->instance.get());
    
    for (auto&& vk_physical_device : vk_physical_devices) {
        std::string name = vk_physical_device.getProperties().deviceName;

        auto& physical_device = physical_devices_.emplace_back(std::make_unique<PhysicalDevice>(name));
        physical_device->impl_ = std::make_unique<PhysicalDevice::Impl>(std::move(vk_physical_device));
    }

    logger->info("Physical devices:");
    for (size_t i = 0; i < physical_devices_.size(); ++i) {
        logger->info(" - [{}] {}", i, physical_devices_[i]->name());
    }

    selectBestPhysicalDevice();
    createLogicalDevice();
}

void Gfx::selectPhysicalDevice(int index) {
    current_physical_device_index_ = std::max(std::min(index, static_cast<int>(physical_devices_.size()) - 1), -1);
    
    if (valid()) {
        logger->info("Selected physical devices: [{}] {}", current_physical_device_index_, physical_device().name());
    } else {
        logger->info("Selected none physical device.");
    }
}

bool Gfx::physical_device_valid() const {
    return current_physical_device_index_ >= 0 && current_physical_device_index_ < static_cast<int>(physical_devices_.size());
}

void Gfx::selectBestPhysicalDevice(int hint_index) {
    auto rateDevice = [](const PhysicalDevice& device, int hint_score) {
        int score = hint_score;
        auto property = device.impl_->vk_physical_device.getProperties();
        auto features = device.impl_->vk_physical_device.getFeatures();

        if (property.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            score += 1000;
        }

        score += property.limits.maxImageDimension2D;

        if (features.geometryShader) {
            score += 100;
        }

        if (features.tessellationShader) {
            score += 100;
        }

        if (!device.impl_->queue_families.graphics_family.has_value()) {
            score = 0;
        }

        return score;
    };

    std::multimap<int, int> candidates;
    for (size_t i = 0; i < physical_devices_.size(); ++i) {
        int hint_score = hint_index == static_cast<int>(i) ? 500 : 0;
        int score = rateDevice(*physical_devices_[i], hint_score);
        candidates.insert(std::make_pair(score, static_cast<int>(i)));
    }

    int index = -1;
    if (!candidates.empty() && candidates.rbegin()->first > 0) {
        index = candidates.rbegin()->second;
    } else {
        logger->error("Failed to find available physical device.");
    }

    selectPhysicalDevice(index);
}

struct LogicalDevice::Impl {
    vk::raii::Device vk_device;
    
    Impl(vk::raii::Device vk_device) 
        : vk_device(std::move(vk_device)) {}
};

LogicalDevice::LogicalDevice()
    : impl_() {}

void Gfx::createLogicalDevice() {
    logger->info("Creating logical device.");
    if (!physical_device_valid()) {
        logger->error("Failed to create logical device because physical device is invalid.");
        return;
    }

    auto& vk_physical_device = physical_device().impl_->vk_physical_device;

    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    std::vector<std::array<float, 1>> queue_priorities;
    if (physical_device().impl_->queue_families.graphics_family.has_value()) {
        vk::DeviceQueueCreateInfo queue_create_info{
            .queueFamilyIndex = physical_device().impl_->queue_families.graphics_family.value()
        };
        queue_priorities.emplace_back(std::array{ 1.0f });
        queue_create_info.setQueuePriorities(queue_priorities.back());
        queue_create_infos.emplace_back(std::move(queue_create_info));
    }

    vk::PhysicalDeviceFeatures enabled_features;

    vk::DeviceCreateInfo device_create_info{
        .pEnabledFeatures = &enabled_features
    };
    device_create_info.setQueueCreateInfos(queue_create_infos);

    vk::raii::Device vk_device = vk_physical_device.createDevice(device_create_info);

    logical_device_ = std::make_unique<LogicalDevice>();
    logical_device_->impl_ = std::make_unique<LogicalDevice::Impl>(std::move(vk_device));

    logger->info("Logical device created.");
}

}