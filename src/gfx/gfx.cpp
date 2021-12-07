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

#include <vector>
#include <algorithm>

#include "engine/engine.h"
#include "platform/platform.h"

namespace {

    auto logger = spdlog::stdout_color_mt("gfx");
    const std::vector<const char*> ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> DebugUtilsExtensions = {
        "VK_EXT_debug_utils"
    };

    bool CheckLayersAvailable(const std::vector<vk::LayerProperties>& availableLayers, const std::vector<const char*>& layerNames) {
        for (auto&& layerName : layerNames) {
            auto found = std::find_if(availableLayers.begin(), availableLayers.end(), 
                [&layerName](const vk::LayerProperties& layer) {
                    return std::string_view(layer.layerName) == std::string_view(layerName);
                });
            if (availableLayers.end() == found) {
                return false;
            }
        }
        return true;
    }

    bool CheckExtensionsAvailable(const std::vector<vk::ExtensionProperties>& availableExtensions, const std::vector<const char*>& extensionNames) {
        for (auto&& extensionName : extensionNames) {
            auto found = std::find_if(availableExtensions.begin(), availableExtensions.end(), 
                [&extensionName](const vk::ExtensionProperties& extension) {
                    return std::string_view(extension.extensionName) == std::string_view(extensionName);
                });
            if (availableExtensions.end() == found) {
                return false;
            }
        }
        return true;
    }

    bool AddDebugUtils(
        std::vector<const char*>& enabledLayerNames, std::vector<const char*>& enabledExtensionNames,
        const std::vector<vk::LayerProperties>& availableLayers, const std::vector<vk::ExtensionProperties>& availableExtensions
        ) {
#ifdef NDEBUG
        return false;
#else
        if (CheckLayersAvailable(availableLayers, ValidationLayers)) {
            enabledLayerNames.insert(enabledLayerNames.end(), ValidationLayers.begin(), ValidationLayers.end());
        } else {
            logger->warn("Debug validation layers unavailable.");
        }
        if (CheckExtensionsAvailable(availableExtensions, DebugUtilsExtensions)) {
            enabledExtensionNames.insert(enabledExtensionNames.end(), DebugUtilsExtensions.begin(), DebugUtilsExtensions.end());
            return true;
        } else {
            logger->warn("Debug utils extentions unavailable.");
            return false;
        }
#endif
    }

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

        std::string typeString;
        typeString.reserve(3);
        auto types = static_cast<vk::DebugUtilsMessageTypeFlagBitsEXT>(messageTypes);
        if (types & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral) {
            typeString += 'G';
        }
        if (types & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation) {
            typeString += 'V';
        }
        if (types & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance) {
            typeString += 'P';
        }

        logger->log(level, "[{}|{}]{}", pCallbackData->pMessageIdName, typeString, pCallbackData->pMessage);

        return false;
    }
                                                    
    std::unique_ptr<vk::raii::DebugUtilsMessengerEXT> CreateDebugMessenger(const vk::raii::Instance& instance) {
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{
            .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | 
                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            .messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                               vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                               vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
            .pfnUserCallback = &DebugMessageHandler
        };
        return std::make_unique<vk::raii::DebugUtilsMessengerEXT>(instance, debugUtilsMessengerCreateInfo, nullptr);
    }

    void AddGlfwUtils(
        std::vector<const char*>& enabledLayerNames, std::vector<const char*>& enabledExtensionNames,
        const std::vector<vk::LayerProperties>& availableLayers, const std::vector<vk::ExtensionProperties>& availableExtensions
    ) {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        enabledExtensionNames.insert(enabledExtensionNames.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);
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
    
    logger->info("Initializing gfx");

    impl_->context = std::make_unique<vk::raii::Context>();

    auto instanceVersion = impl_->context->enumerateInstanceVersion();
    logger->info("Instance version: {}.{}.{}", VK_API_VERSION_MAJOR(instanceVersion), 
        VK_API_VERSION_MINOR(instanceVersion), VK_API_VERSION_PATCH(instanceVersion));
    
    std::vector<vk::ExtensionProperties> availableExtensions = impl_->context->enumerateInstanceExtensionProperties();
    logger->info("{} extensions available:", availableExtensions.size());
    for (auto&& extension : availableExtensions) {
        logger->info(" - {} ({}.{}.{})", 
            extension.extensionName, VK_API_VERSION_MAJOR(extension.specVersion), 
            VK_API_VERSION_MINOR(extension.specVersion), VK_API_VERSION_PATCH(extension.specVersion));
    }
    
    std::vector<vk::LayerProperties> availableLayers = impl_->context->enumerateInstanceLayerProperties();
    logger->info("{} layers available:", availableLayers.size());
    for (auto&& layer : availableLayers) {
        logger->info(" - {} ({}.{}.{}, {}.{}.{}) {}", 
            layer.layerName, VK_API_VERSION_MAJOR(layer.specVersion), 
            VK_API_VERSION_MINOR(layer.specVersion), VK_API_VERSION_PATCH(layer.specVersion), 
            VK_API_VERSION_MAJOR(layer.implementationVersion), VK_API_VERSION_MINOR(layer.implementationVersion), 
            VK_API_VERSION_PATCH(layer.implementationVersion), layer.description);
    }

    const Engine& engine = Engine::get();

    vk::ApplicationInfo applicationInfo{
        .pApplicationName   = app.name().c_str(),
        .applicationVersion = VK_MAKE_VERSION(app.major_version(), app.minor_version(), app.patch_version()),
        .pEngineName        = engine.name().c_str(),
        .engineVersion      = VK_MAKE_VERSION(engine.major_version(), engine.minor_version(), engine.patch_version()),
        .apiVersion         = VK_API_VERSION_1_0
    };

    std::vector<const char*> enabledLayerNames;
    std::vector<const char*> enabledExtensionNames;

    // Adding debug layers and extensions
    const bool debugUtilsAdded = AddDebugUtils(enabledLayerNames, enabledExtensionNames, availableLayers, availableExtensions);

    // Adding GLFW required extensions
    AddGlfwUtils(enabledLayerNames, enabledExtensionNames, availableLayers, availableExtensions);

    logger->info("Enabled layers:");
    for (auto&& layerName : enabledLayerNames) {
        logger->info(" - {}", layerName);
    }

    logger->info("Enabled extensions:");
    for (auto&& extensionName : enabledExtensionNames) {
        logger->info(" - {}", extensionName);
    }

    vk::InstanceCreateInfo instanceCreateInfo{
        .pApplicationInfo = &applicationInfo
    };
    instanceCreateInfo.setPEnabledLayerNames(enabledLayerNames);
    instanceCreateInfo.setPEnabledExtensionNames(enabledExtensionNames);

    impl_->instance = std::make_unique<vk::raii::Instance>(*impl_->context, instanceCreateInfo);

    if (debugUtilsAdded) {
        impl_->debug_messenger = CreateDebugMessenger(*impl_->instance.get());
    }

    vk::raii::PhysicalDevices physicalDevices(*impl_->instance.get());
    logger->info("Physical devices:");
    for (auto&& physicalDevice : physicalDevices) {
        logger->info(" - {}", physicalDevice.getProperties().deviceName);
    }
}

Gfx::~Gfx() {
    logger->info("Destroying gfx");
    impl_->debug_messenger.release();
    impl_->instance.release();
    impl_->context.release();
}

}