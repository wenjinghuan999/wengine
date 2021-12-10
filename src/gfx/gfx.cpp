#include "gfx/gfx.h"
#include "platform-private.h"

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
#include <string>
#include <string_view>
#include <tuple>

#include "engine/engine.h"
#include "platform/platform.h"

namespace wg {
namespace gfx_features {

const char* const FEATURE_NAMES[_NUM_FEATURES] = {
    "_platform_required",
    "_must_enable_if_valid",
    "_debug_utils"
};

}
}

namespace {

auto logger = spdlog::stdout_color_mt("gfx");

inline void AppendRawStrings(std::vector<const char*>& to, const std::vector<const char*>& from) {
    for (const char* s : from) {
        if (std::find_if(to.begin(), to.end(), [s](const char* t){
                return std::string_view(s) == std::string_view(t);
            }) == to.end()) {
            to.push_back(s);
        }
    }
}

struct VulkanFeatures {
    std::vector<const char*> instance_layers;
    std::vector<const char*> instance_extensions;
    std::vector<const char*> device_layers;
    std::vector<const char*> device_extensions;
    vk::PhysicalDeviceFeatures device_features;
    using CheckPropertiesAndFeaturesFunc 
        = std::function<bool(const vk::PhysicalDeviceProperties&, const vk::PhysicalDeviceFeatures&)>;
    CheckPropertiesAndFeaturesFunc check_properties_and_features_func;

    using SetFeaturesFunc = std::function<void(vk::PhysicalDeviceFeatures&)>;
    SetFeaturesFunc set_feature_func;
public:
    VulkanFeatures& append(const VulkanFeatures& other) {
        AppendRawStrings(instance_layers, other.instance_layers);
        AppendRawStrings(instance_extensions, other.instance_extensions);
        AppendRawStrings(device_layers, other.device_layers);
        AppendRawStrings(device_extensions, other.device_extensions);
        if (other.set_feature_func) {
            other.set_feature_func(device_features);
        }
        return *this;
    }
};

struct AvailableVulkanFeatures {
    std::vector<vk::LayerProperties> instance_layers;
    std::vector<vk::ExtensionProperties> instance_extensions;
    std::vector<vk::LayerProperties> device_layers;
    std::vector<vk::ExtensionProperties> device_extensions;
    vk::PhysicalDeviceProperties device_properties;
    vk::PhysicalDeviceFeatures device_features;
public:
    void getInstanceFeatures(const vk::raii::Context& context) {
        instance_layers = context.enumerateInstanceLayerProperties();
        instance_extensions = context.enumerateInstanceExtensionProperties();
    }
    void getDeviceFeatures(const vk::raii::PhysicalDevice& device) {
        device_layers = device.enumerateDeviceLayerProperties();
        device_extensions = device.enumerateDeviceExtensionProperties();
        device_properties = device.getProperties();
        device_features = device.getFeatures();
    }
public:
    bool checkInstanceAvailable(const VulkanFeatures& feature) const {
        for (auto&& layer_name : feature.instance_layers) {
            if (!CheckLayerContains(instance_layers, layer_name)) {
                return false;
            }
        }
        for (auto&& extension_name : feature.instance_extensions) {
            if (!CheckExtensionContains(instance_extensions, extension_name)) {
                return false;
            }
        }
        return true;
    }
    bool checkDeviceAvailable(const VulkanFeatures& feature) const {
        for (auto&& layer_name : feature.device_layers) {
            if (!CheckLayerContains(device_layers, layer_name)) {
                return false;
            }
        }
        for (auto&& extension_name : feature.device_extensions) {
            if (!CheckExtensionContains(device_extensions, extension_name)) {
                return false;
            }
        }
        return true;
    }
private:
    static bool CheckLayerContains(const std::vector<vk::LayerProperties>& layers, const char* layer_name) {
        return std::find_if(layers.begin(), layers.end(), 
            [layer_name](const vk::LayerProperties& layer) {
                return std::string_view(layer.layerName) == std::string_view(layer_name);
            }) != layers.end();
    }
    static bool CheckExtensionContains(const std::vector<vk::ExtensionProperties>& extensions, const char* extension_name) {
        return std::find_if(extensions.begin(), extensions.end(), 
            [extension_name](const vk::ExtensionProperties& extension) {
                return std::string_view(extension.extensionName) == std::string_view(extension_name);
            }) != extensions.end();
    }
};

VulkanFeatures GetGlfwRequiredFeatures() {
    uint32_t glfw_extension_count = 0;
    const char** glfw_extension_names = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    VulkanFeatures glfw_features;
    glfw_features.instance_extensions = std::vector<const char*>(glfw_extension_names, glfw_extension_names + glfw_extension_count);
    
    static bool first_time = true;
    if (first_time) {
        logger->debug("GLFW requires {} extensions:", glfw_extension_count);
        for (const char* extension_name : glfw_features.instance_extensions) {
            logger->debug(" - {}", extension_name);
        }
        first_time = false;
    }
    return glfw_features;
}

VulkanFeatures GetVulkanFeatures(wg::gfx_features::FeatureId feature) {
    switch (feature) {
        case wg::gfx_features::_platform_required:
            return GetGlfwRequiredFeatures();
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

#ifndef NDEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageHandler(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    VkDebugUtilsMessengerCallbackDataEXT const * pCallbackData, void * /*pUserData*/ ) {
    
    static auto vulkan_logger = spdlog::stdout_color_mt("vulkan");

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

    vulkan_logger->log(level, "[{}][{}] {}", 
        pCallbackData->pMessageIdName ? pCallbackData->pMessageIdName : "-", type_string, pCallbackData->pMessage);

    return false;
}
                                                
vk::raii::DebugUtilsMessengerEXT CreateDebugMessenger(const vk::raii::Instance& instance) {
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
    return instance.createDebugUtilsMessengerEXT(debug_utils_messenger_create_info);
}
#endif

} // unnamed namespace

namespace wg {

struct Gfx::Impl {
    vk::raii::Context context;
    vk::raii::Instance instance{nullptr};
    vk::raii::DebugUtilsMessengerEXT debug_messenger{nullptr};
};

Gfx::Gfx(const App& app)
    : impl_(std::make_unique<Gfx::Impl>()) {
    
#ifndef NDEBUG
    logger->set_level(spdlog::level::debug);
#endif
    logger->info("Initializing gfx.");

    auto instance_version = impl_->context.enumerateInstanceVersion();
    logger->info("Instance version: {}.{}.{}", VK_API_VERSION_MAJOR(instance_version), 
        VK_API_VERSION_MINOR(instance_version), VK_API_VERSION_PATCH(instance_version));
    
    // Check and enable available vulkan features
    AvailableVulkanFeatures available_features;
    available_features.getInstanceFeatures(impl_->context);

    logger->info("{} instance layers available.", available_features.instance_layers.size());
    for (auto&& layer : available_features.instance_layers) {
        logger->debug(" - {} ({}.{}.{}, {}.{}.{}) {}", 
            layer.layerName, VK_API_VERSION_MAJOR(layer.specVersion), 
            VK_API_VERSION_MINOR(layer.specVersion), VK_API_VERSION_PATCH(layer.specVersion), 
            VK_API_VERSION_MAJOR(layer.implementationVersion), VK_API_VERSION_MINOR(layer.implementationVersion), 
            VK_API_VERSION_PATCH(layer.implementationVersion), layer.description);
    }

    logger->info("{} instance extensions available.", available_features.instance_extensions.size());
    for (auto&& extension : available_features.instance_extensions) {
        logger->debug(" - {} ({}.{}.{})", 
            extension.extensionName, VK_API_VERSION_MAJOR(extension.specVersion), 
            VK_API_VERSION_MINOR(extension.specVersion), VK_API_VERSION_PATCH(extension.specVersion));
    }

    const Engine& engine = Engine::get();

    vk::ApplicationInfo application_info{
        .pApplicationName   = app.name().c_str(),
        .applicationVersion = VK_MAKE_VERSION(app.major_version(), app.minor_version(), app.patch_version()),
        .pEngineName        = engine.name().c_str(),
        .engineVersion      = VK_MAKE_VERSION(engine.major_version(), engine.minor_version(), engine.patch_version()),
        .apiVersion         = VK_API_VERSION_1_0
    };

    GfxFeaturesManager& gfx_features_manager = GfxFeaturesManager::get();
    GfxFeaturesManager::_AddFeatureImpl(gfx_features::_platform_required, gfx_features_manager.instance_enabled_);
    GfxFeaturesManager::_AddFeatureImpl(gfx_features::_platform_required, gfx_features_manager.required_);
    GfxFeaturesManager::_AddFeatureImpl(gfx_features::_must_enable_if_valid, gfx_features_manager.instance_enabled_);
    GfxFeaturesManager::_AddFeatureImpl(gfx_features::_must_enable_if_valid, gfx_features_manager.defaults_);
#ifndef NDEBUG
    // Adding debug layers and extensions
    GfxFeaturesManager::_AddFeatureImpl(gfx_features::_debug_utils, gfx_features_manager.instance_enabled_);
    GfxFeaturesManager::_AddFeatureImpl(gfx_features::_debug_utils, gfx_features_manager.defaults_);
#endif

    for (int feature_id = 0; feature_id < gfx_features::NUM_FEATURES; ++feature_id) {
        GfxFeaturesManager::_AddFeatureImpl(
            static_cast<gfx_features::FeatureId>(feature_id), gfx_features_manager.instance_enabled_);
        // TODO: add to default
    }

    VulkanFeatures enabled_features;

    // Required features must be available
    for (auto feature_id : gfx_features_manager.required_) {
        VulkanFeatures feature = GetVulkanFeatures(feature_id);
        if (!available_features.checkInstanceAvailable(feature)) {
            logger->error("Gfx feature {} is required but not available on instance.", gfx_features::FEATURE_NAMES[feature_id]);
        }
        // May crash later
        enabled_features.append(feature);
    }

    // Instance features are enabled if available
    for (auto feature_id : gfx_features_manager.instance_enabled_) {
        VulkanFeatures feature = GetVulkanFeatures(feature_id);
        if (!available_features.checkInstanceAvailable(feature)) {
            logger->info("Gfx feature {} is not available on instance.", gfx_features::FEATURE_NAMES[feature_id]);
            GfxFeaturesManager::_RemoveFeatureImpl(feature_id, gfx_features_manager.instance_enabled_);
            GfxFeaturesManager::_RemoveFeatureImpl(feature_id, gfx_features_manager.defaults_);
        } else {
            enabled_features.append(feature);
        }
    }

    logger->info("Enabled engine features:");
    for (auto feature_id : gfx_features_manager.instance_enabled_) {
        logger->info(" - {}", gfx_features::FEATURE_NAMES[feature_id]);
    }

    logger->info("Enabled layers:");
    for (auto&& layer_name : enabled_features.instance_layers) {
        logger->info(" - {}", layer_name);
    }

    logger->info("Enabled extensions:");
    for (auto&& extension_name : enabled_features.instance_extensions) {
        logger->info(" - {}", extension_name);
    }

    auto instance_create_info = vk::InstanceCreateInfo{
        .pApplicationInfo = &application_info
    }   .setPEnabledLayerNames(enabled_features.instance_layers)
        .setPEnabledExtensionNames(enabled_features.instance_extensions);

    impl_->instance = impl_->context.createInstance(instance_create_info);

#ifndef NDEBUG
    impl_->debug_messenger = CreateDebugMessenger(impl_->instance);
#endif

    // Devices
    updatePhysicalDevices();
}

Gfx::~Gfx() {
    logger->info("Destroying gfx.");
}

bool Gfx::valid() const {
    return physical_device_valid();
}

struct Surface::Impl {
    vk::raii::SurfaceKHR vk_surface;
};

Surface::Surface(std::shared_ptr<Window> window)
    : window_(std::move(window)), impl_() {}

std::shared_ptr<Surface> Gfx::createSurface(std::shared_ptr<Window> window) {
    auto surface = std::make_shared<Surface>(window);

    VkSurfaceKHR vk_surface;
    VkResult err = glfwCreateWindowSurface(*impl_->instance, window->impl_->glfw_window, NULL, &vk_surface);
    if (err != VK_SUCCESS) {
        spdlog::error("Failed to create surface for window \"{}\". Error code {}.", window->title_, err);
    }
    surface->impl_->vk_surface = vk::raii::SurfaceKHR(impl_->instance, vk_surface);

    surfaces_.push_back(surface);
    return surface;
}

} // namespace wg

namespace {

struct QueueRequirements {
    vk::QueueFlags flags;
    
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> compute_family;
    std::optional<uint32_t> transfer_family;
    std::optional<uint32_t> sparse_binding_family;
};

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

} // unnamed namespace


namespace wg {

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

    auto vk_physical_devices = impl_->instance.enumeratePhysicalDevices();
    
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

    // Check and enable available device features
    AvailableVulkanFeatures available_features;
    available_features.getDeviceFeatures(vk_physical_device);

    logger->info("{} device layers available.", available_features.device_layers.size());
    for (auto&& layer : available_features.device_layers) {
        logger->debug(" - {} ({}.{}.{}, {}.{}.{}) {}", 
            layer.layerName, VK_API_VERSION_MAJOR(layer.specVersion), 
            VK_API_VERSION_MINOR(layer.specVersion), VK_API_VERSION_PATCH(layer.specVersion), 
            VK_API_VERSION_MAJOR(layer.implementationVersion), VK_API_VERSION_MINOR(layer.implementationVersion), 
            VK_API_VERSION_PATCH(layer.implementationVersion), layer.description);
    }

    logger->info("{} device extensions available.", available_features.device_extensions.size());
    for (auto&& extension : available_features.device_extensions) {
        logger->debug(" - {} ({}.{}.{})", 
            extension.extensionName, VK_API_VERSION_MAJOR(extension.specVersion), 
            VK_API_VERSION_MINOR(extension.specVersion), VK_API_VERSION_PATCH(extension.specVersion));
    }

    VulkanFeatures enabled_features;
    GfxFeaturesManager& gfx_features_manager = GfxFeaturesManager::get();

    for (auto feature_id : gfx_features_manager.instance_enabled_) {
        VulkanFeatures feature = GetVulkanFeatures(feature_id);
        const bool enabled = 
            GfxFeaturesManager::_Contains(feature_id, gfx_features_manager.user_enabled_) ||
            (!GfxFeaturesManager::_Contains(feature_id, gfx_features_manager.user_disabled_) && 
                GfxFeaturesManager::_Contains(feature_id, gfx_features_manager.defaults_));

        if (enabled && !available_features.checkDeviceAvailable(feature)) {
            logger->info("Gfx feature {} is not available on device.", gfx_features::FEATURE_NAMES[feature_id]);
        } else if (enabled) {
            GfxFeaturesManager::_AddFeatureImpl(feature_id, gfx_features_manager.enabled_);
            enabled_features.append(feature);
        }
    }

    logger->info("Enabled device features:");
    for (auto feature_id : gfx_features_manager.enabled_) {
        logger->info(" - {}", gfx_features::FEATURE_NAMES[feature_id]);
    }

    logger->info("Enabled device layers:");
    for (auto&& layer_name : enabled_features.device_layers) {
        logger->info(" - {}", layer_name);
    }

    logger->info("Enabled device extensions:");
    for (auto&& extension_name : enabled_features.device_extensions) {
        logger->info(" - {}", extension_name);
    }

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

    auto device_create_info = vk::DeviceCreateInfo{}
        .setPEnabledLayerNames(enabled_features.device_layers)
        .setPEnabledExtensionNames(enabled_features.device_extensions)
        .setPEnabledFeatures(&enabled_features.device_features)
        .setQueueCreateInfos(queue_create_infos);

    vk::raii::Device vk_device = vk_physical_device.createDevice(device_create_info);

    logical_device_ = std::make_unique<LogicalDevice>();
    logical_device_->impl_ = std::make_unique<LogicalDevice::Impl>(std::move(vk_device));

    logger->info("Logical device created.");
}

void GfxFeaturesManager::_AddFeatureImpl(gfx_features::FeatureId feature, 
    std::vector<gfx_features::FeatureId>& features) {
    
    if (std::find(features.begin(), features.end(), feature) == features.end()) {
        features.push_back(feature);
    }
}

void GfxFeaturesManager::_RemoveFeatureImpl(gfx_features::FeatureId feature, 
    std::vector<gfx_features::FeatureId>& features) {
    
    std::ignore = std::remove(features.begin(), features.end(), feature);
}

bool GfxFeaturesManager::_Contains(gfx_features::FeatureId feature, 
    const std::vector<gfx_features::FeatureId>& features) {
    
    return std::find(features.begin(), features.end(), feature) != features.end();
}

} // namespace wg