#include "platform/inc/platform.inc"

#include <algorithm>
#include <map>
#include <vector>
#include <string_view>
#include <tuple>
#include <utility>

#include "gfx/gfx.h"
#include "common/logger.h"
#include "common/constants.h"
#include "gfx/gfx-constants.h"
#include "platform/platform.h"
#include "platform/inc/window-private.h"
#include "gfx-private.h"

namespace wg {

namespace gfx_features {

const char* const FEATURE_NAMES[NUM_FEATURES_TOTAL] = {
    "window_surface",
    "separate_transfer",
    "_must_enable_if_valid",
    "_debug_utils"
};

} // namespace gfx_features

} // namespace wg

namespace {

[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

inline void AppendRawStrings(std::vector<const char*>& to, const std::vector<const char*>& from) {
    for (const char* s : from) {
        if (std::find_if(
            to.begin(), to.end(), [s](const char* t) {
                return std::string_view(s) == std::string_view(t);
            }
        ) == to.end()) {
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
    std::array<int, wg::gfx_queues::NUM_QUEUES> device_queues{};

    using CheckPropertiesAndFeaturesFunc = std::function<bool(const vk::PhysicalDeviceProperties&, const vk::PhysicalDeviceFeatures&)>;
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
        for (int i = 0; i < device_queues.size(); ++i) {
            device_queues[i] += other.device_queues[i];
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
    std::array<int, wg::gfx_queues::NUM_QUEUES> device_queues{};

public:
    void getInstanceFeatures(const vk::raii::Context& context) {
        instance_layers = context.enumerateInstanceLayerProperties();
        instance_extensions = context.enumerateInstanceExtensionProperties();
    }
    void getDeviceFeatures(
        const vk::raii::PhysicalDevice& device,
        std::array<int, wg::gfx_queues::NUM_QUEUES> num_queues_total
    ) {
        device_layers = device.enumerateDeviceLayerProperties();
        device_extensions = device.enumerateDeviceExtensionProperties();
        device_properties = device.getProperties();
        device_features = device.getFeatures();
        device_queues = num_queues_total;
    }
    [[nodiscard]] bool checkInstanceAvailable(const VulkanFeatures& feature) const {
        return std::all_of(
            feature.instance_layers.begin(), feature.instance_layers.end(),
            [this](const char* layer_name) {
                return CheckLayerContains(instance_layers, layer_name);
            }
        ) && std::all_of(
            feature.instance_extensions.begin(), feature.instance_extensions.end(),
            [this](const char* extension_name) {
                return CheckExtensionContains(instance_extensions, extension_name);
            }
        );
    }
    [[nodiscard]] bool checkDeviceAvailable(const VulkanFeatures& feature) const {
        bool layers_and_extensions_available =
            std::all_of(
                feature.device_layers.begin(), feature.device_layers.end(),
                [this](const char* layer_name) {
                    return CheckLayerContains(device_layers, layer_name);
                }
            ) && std::all_of(
                feature.device_extensions.begin(), feature.device_extensions.end(),
                [this](const char* extension_name) {
                    return CheckExtensionContains(device_extensions, extension_name);
                }
            );
        if (!layers_and_extensions_available) {
            return false;
        }
        for (int i = 0; i < wg::gfx_queues::NUM_QUEUES; ++i) {
            if (device_queues[i] < feature.device_queues[i]) {
                return false;
            }
        }
        if (feature.check_properties_and_features_func) {
            if (!feature.check_properties_and_features_func(device_properties, device_features)) {
                return false;
            }
        }
        return true;
    }

private:
    static bool CheckLayerContains(const std::vector<vk::LayerProperties>& layers, const char* layer_name) {
        return std::find_if(
            layers.begin(), layers.end(),
            [layer_name](const vk::LayerProperties& layer) {
                return static_cast<std::string_view>(layer.layerName) == std::string_view(layer_name);
            }
        ) != layers.end();
    }
    static bool CheckExtensionContains(const std::vector<vk::ExtensionProperties>& extensions, const char* extension_name) {
        return std::find_if(
            extensions.begin(), extensions.end(),
            [extension_name](const vk::ExtensionProperties& extension) {
                return static_cast<std::string_view>(extension.extensionName) == std::string_view(extension_name);
            }
        ) != extensions.end();
    }
};

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

[[nodiscard]] VulkanFeatures GetVulkanFeatures(wg::gfx_features::FeatureId feature) {
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
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void* /*pUserData*/ ) {

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

    vulkan_logger->log(
        level, "[{}][{}] {}",
        pCallbackData->pMessageIdName ? pCallbackData->pMessageIdName : "-", type_string, pCallbackData->pMessage
    );

    return false;
}

[[nodiscard]] vk::raii::DebugUtilsMessengerEXT CreateDebugMessenger(const vk::raii::Instance& instance) {
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

std::shared_ptr<Gfx> Gfx::Create(const App& app) {
    return std::shared_ptr<Gfx>(new Gfx(app));
}

Gfx::Gfx(const App& app)
    : impl_(std::make_unique<Gfx::Impl>()) {

    impl_->gfx = this;
    logger().info("Initializing gfx.");

    auto instance_version = impl_->context.enumerateInstanceVersion();
    logger().info(
        "Instance version: {}.{}.{}", VK_API_VERSION_MAJOR(instance_version),
        VK_API_VERSION_MINOR(instance_version), VK_API_VERSION_PATCH(instance_version)
    );

    // Check and enable available vulkan features
    AvailableVulkanFeatures available_features;
    available_features.getInstanceFeatures(impl_->context);

    logger().info(
        "{} instance layers available.", available_features.instance_layers.size()
    );
    for (auto&& layer : available_features.instance_layers) {
        logger().debug(
            " - {} ({}.{}.{}, {}.{}.{}) {}",
            layer.layerName, VK_API_VERSION_MAJOR(layer.specVersion),
            VK_API_VERSION_MINOR(layer.specVersion), VK_API_VERSION_PATCH(layer.specVersion),
            VK_API_VERSION_MAJOR(layer.implementationVersion), VK_API_VERSION_MINOR(layer.implementationVersion),
            VK_API_VERSION_PATCH(layer.implementationVersion), layer.description
        );
    }

    logger().info(
        "{} instance extensions available.", available_features.instance_extensions
            .size()
    );
    for (auto&& extension : available_features.instance_extensions) {
        logger().debug(
            " - {} ({}.{}.{})",
            extension.extensionName, VK_API_VERSION_MAJOR(extension.specVersion),
            VK_API_VERSION_MINOR(extension.specVersion), VK_API_VERSION_PATCH(extension.specVersion)
        );
    }

    const EngineConstants& engine_constants = EngineConstants::Get();

    vk::ApplicationInfo application_info{
        .pApplicationName   = app.name().c_str(),
        .applicationVersion = VK_MAKE_VERSION(app.major_version(), app.minor_version(), app.patch_version()),
        .pEngineName        = engine_constants.name().c_str(),
        .engineVersion      = VK_MAKE_VERSION(engine_constants.major_version(),
            engine_constants.minor_version(), engine_constants.patch_version()),
        .apiVersion         = VK_API_VERSION_1_0
    };

    GfxFeaturesManager& gfx_features_manager = GfxFeaturesManager::Get();
    GfxFeaturesManager::AddFeatureImpl(gfx_features::_must_enable_if_valid, gfx_features_manager.instance_enabled_);
    GfxFeaturesManager::AddFeatureImpl(gfx_features::_must_enable_if_valid, gfx_features_manager.defaults_);
#ifndef NDEBUG
    // Adding debug layers and extensions
    GfxFeaturesManager::AddFeatureImpl(gfx_features::_debug_utils, gfx_features_manager.instance_enabled_);
    GfxFeaturesManager::AddFeatureImpl(gfx_features::_debug_utils, gfx_features_manager.defaults_);
#endif

    for (int feature_id = 0; feature_id < gfx_features::NUM_FEATURES; ++feature_id) {
        GfxFeaturesManager::AddFeatureImpl(
            static_cast<gfx_features::FeatureId>(feature_id), gfx_features_manager.instance_enabled_
        );
        // TODO: add to default
    }

    VulkanFeatures enabled_features;

    // Required features must be available
    for (auto feature_id : gfx_features_manager.required_) {
        VulkanFeatures feature = GetVulkanFeatures(feature_id);
        if (!available_features.checkInstanceAvailable(feature)) {
            logger().error("Gfx feature {} is required but not available on instance.", gfx_features::FEATURE_NAMES[feature_id]);
        }
        // May crash later
        enabled_features.append(feature);
    }

    // Instance features are enabled if available
    for (auto feature_id : gfx_features_manager.instance_enabled_) {
        VulkanFeatures feature = GetVulkanFeatures(feature_id);
        if (!available_features.checkInstanceAvailable(feature)) {
            logger().info("Gfx feature {} is not available on instance.", gfx_features::FEATURE_NAMES[feature_id]);
            GfxFeaturesManager::RemoveFeatureImpl(feature_id, gfx_features_manager.instance_enabled_);
            GfxFeaturesManager::RemoveFeatureImpl(feature_id, gfx_features_manager.defaults_);
        } else {
            enabled_features.append(feature);
        }
    }

    logger().info("Enabled engine features:");
    for (auto feature_id : gfx_features_manager.instance_enabled_) {
        logger().info(" - {}", gfx_features::FEATURE_NAMES[feature_id]);
    }

    logger().info("Enabled layers:");
    for (auto&& layer_name : enabled_features.instance_layers) {
        logger().info(" - {}", layer_name);
    }

    logger().info("Enabled extensions:");
    for (auto&& extension_name : enabled_features.instance_extensions) {
        logger().info(" - {}", extension_name);
    }

    auto instance_create_info = vk::InstanceCreateInfo{
        .pApplicationInfo = &application_info
    }
        .setPEnabledLayerNames(enabled_features.instance_layers)
        .setPEnabledExtensionNames(enabled_features.instance_extensions);

    impl_->instance = impl_->context.createInstance(instance_create_info);

#ifndef NDEBUG
    impl_->debug_messenger = CreateDebugMessenger(impl_->instance);
#endif

    // Devices
    updatePhysicalDevices();
}

Gfx::~Gfx() {
    waitDeviceIdle();
    logger().info("Destroying gfx.");
}

bool Gfx::valid() const {
    return physical_device_valid();
}

PhysicalDevice::PhysicalDevice(std::string name)
    : name_(std::move(name)), impl_() {}

void Gfx::updatePhysicalDevices() {
    physical_devices_.clear();

    auto vk_physical_devices = impl_->instance
        .enumeratePhysicalDevices();

    for (auto&& vk_physical_device : vk_physical_devices) {
        std::string name = vk_physical_device.getProperties()
            .deviceName;

        auto& physical_device = physical_devices_.emplace_back(std::make_unique<PhysicalDevice>(name));
        physical_device->impl_ = std::make_unique<PhysicalDevice::Impl>(std::move(vk_physical_device));
    }

    logger().info("Physical devices:");
    for (size_t i = 0; i < physical_devices_.size(); ++i) {
        logger().info(" - [{}] {}", i, physical_devices_[i]->name());
    }
}

void Gfx::selectPhysicalDevice(int index) {
    current_physical_device_index_ = std::max(std::min(index, static_cast<int>(physical_devices_.size()) - 1), -1);

    if (valid()) {
        logger().info("Selected physical devices: [{}] {}", current_physical_device_index_, physical_device().name());
    } else {
        logger().info("Selected none physical device.");
    }
}

bool Gfx::physical_device_valid() const {
    return current_physical_device_index_ >= 0 && current_physical_device_index_ < static_cast<int>(physical_devices_.size());
}

void Gfx::selectBestPhysicalDevice(int hint_index) {
    const auto& gfx_features_manager = GfxFeaturesManager::Get();
    auto features_required = gfx_features_manager.features_required();
    auto queues_required = gfx_features_manager.queues_required();

    auto rateDevice = [this, &features_required, &queues_required]
        (const PhysicalDevice& device, int hint_score) {

        int score = hint_score;
        auto property = device.impl_
            ->vk_physical_device
            .getProperties();
        auto features = device.impl_
            ->vk_physical_device
            .getFeatures();

        if (property.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            score += 1000;
        }

        score += static_cast<int>(property.limits
            .maxImageDimension2D);

        if (features.geometryShader) {
            score += 100;
        }

        if (features.tessellationShader) {
            score += 100;
        }

        for (int i = 0; i < gfx_queues::NUM_QUEUES; ++i) {
            if (queues_required[i] > device.impl_
                ->num_queues_total[i]) {
                return 0;
            }
        }

        for (auto& window_surface : impl_->window_surfaces_) {
            bool has_any_family = false;
            const auto& surface = window_surface.surface;
            auto window = surface->window_
                .lock();
            if (!window) {
                continue;
            }
            for (uint32_t family_index = 0;
                 family_index < device.impl_
                     ->num_queues
                     .size();
                 ++family_index) {
                if (device.impl_
                    ->vk_physical_device
                    .getSurfaceSupportKHR(
                        family_index, *surface->impl_
                            ->vk_surface
                    )) {
                    has_any_family = true;
                    break;
                }
            }
            if (!has_any_family) {
                logger().info(
                    "Physical device {} does not support surface of window \"{}\" (no suitable queue family).",
                    device.name(), window->title()
                );
                return 0;
            }
            if (device.impl_
                ->vk_physical_device
                .getSurfaceFormatsKHR(
                    *surface->impl_
                        ->vk_surface
                )
                .empty()) {
                logger().info(
                    "Physical device {} does not support surface of window \"{}\" (no format).",
                    device.name(), window->title()
                );
                return 0;
            }
            if (device.impl_
                ->vk_physical_device
                .getSurfacePresentModesKHR(
                    *surface->impl_
                        ->vk_surface
                )
                .empty()) {
                logger().info(
                    "Physical device {} does not support surface of window \"{}\" (no present mode).",
                    device.name(), window->title()
                );
                return 0;
            }
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
    if (!candidates.empty() && candidates.rbegin()
        ->first > 0) {
        index = candidates.rbegin()
            ->second;
    } else {
        logger().error("Failed to find available physical device.");
    }

    selectPhysicalDevice(index);
}

PhysicalDevice::Impl::Impl(vk::raii::PhysicalDevice vk_physical_device)
    : vk_physical_device(std::move(vk_physical_device)) {
    auto queue_families = this->vk_physical_device
        .getQueueFamilyProperties();
    std::fill(num_queues_total.begin(), num_queues_total.end(), 0);
    num_queues = std::vector<int>(queue_families.size(), 0);
    queue_supports = std::vector<std::bitset<gfx_queues::NUM_QUEUES>>(queue_families.size(), 0);
    for (uint32_t queue_family_index = 0;
         queue_family_index < static_cast<uint32_t>(queue_families.size());
         ++queue_family_index) {

        const auto& queue_family = queue_families[queue_family_index];
        num_queues[queue_family_index] = static_cast<int>(queue_family.queueCount);

        for (int i = 0; i < gfx_queues::NUM_QUEUES; ++i) {
            auto queue_id = static_cast<gfx_queues::QueueId>(i);
            vk::QueueFlags required_flags = GetRequiredQueueFlags(queue_id);

            if (queue_family.queueFlags & required_flags) {
                queue_supports[queue_family_index][i] = true;
                num_queues_total[i] += num_queues[queue_family_index];
            }
        }
    }
}

int PhysicalDevice::Impl::findMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags property_flags) {
    auto memory_properties = vk_physical_device.getMemoryProperties();
    for (int i = 0; i < static_cast<int>(memory_properties.memoryTypeCount); i++) {
        if (requirements.memoryTypeBits & (1 << i) &&
            (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
            return i;
        }
    }
    return -1;
}

bool PhysicalDevice::Impl::allocateQueues(
    std::array<int, gfx_queues::NUM_QUEUES> required_queues,
    const std::vector<vk::SurfaceKHR>& surfaces,
    std::array<std::vector<std::pair<uint32_t, uint32_t>>, gfx_queues::NUM_QUEUES>& out_queue_index_remap
) {
    bool num_queues_valid = [this]() {
        for (size_t i = 0; i < num_queues.size(); ++i) {
            if (num_queues[i] <= 0) {
                return false;
            }
        }
        return !num_queues.empty();
    }();
    if (!num_queues_valid) {
        logger().error("Cannot allocate queues because num_queues is invalid.");
        return false;
    }

    // 1. Find out queue support considering surfaces
    std::vector<std::bitset<gfx_queues::NUM_QUEUES>> real_queue_supports = queue_supports;
    for (uint32_t family_index = 0; family_index < num_queues.size(); ++family_index) {
        // Fill queue_counts[queue_family_index][NUM_QUEUES] = num of queues available
        for (int i = 0; i < gfx_queues::NUM_QUEUES; ++i) {
            auto queue_id = static_cast<gfx_queues::QueueId>(i);
            if (!checkQueueSupport(family_index, queue_id, surfaces)) {
                real_queue_supports[family_index][i] = false;
            }
        }
    }

    // 2. Try allocate queues. If failed, reduce queue counts one by one.
    std::vector<std::map<gfx_queues::QueueId, int>> allocated_queue_counts;
    bool succeeded = false;
    bool succeeded_allocating_all = true;
    int last_reduced_queue_id = 0;
    while (!succeeded) {
        succeeded = tryAllocateQueues(required_queues, real_queue_supports, allocated_queue_counts);
        if (succeeded) {
            break;
        }
        succeeded_allocating_all = false;
        bool reduced = false;
        for (int i = 1; i <= gfx_queues::NUM_QUEUES; ++i) {
            int queue_id_to_reduce = (last_reduced_queue_id + i) % gfx_queues::NUM_QUEUES;
            if (required_queues[queue_id_to_reduce] > 1) {
                required_queues[queue_id_to_reduce] -= 1;
                reduced = true;
                last_reduced_queue_id = queue_id_to_reduce;
                break;
            }
        }
        if (!reduced) {
            break;
        }
    };

    // 3. If we still failed after reducing queue counts, we can only allow duplicated queues
    // Now just allocate required queues one by one
    if (!succeeded) {
        succeeded_allocating_all = false;
        uint32_t last_family_index = 0;
        std::vector<size_t> queue_index_in_family(num_queues.size());
        allocated_queue_counts.clear();
        allocated_queue_counts.resize(num_queues.size());
        for (int i = 0; i < gfx_queues::NUM_QUEUES; ++i) {
            auto queue_id = static_cast<gfx_queues::QueueId>(i);
            if (required_queues[i] == 0) {
                continue;
            }

            bool allocated = false;
            for (uint32_t j = 0; j < num_queues.size(); ++j) {
                uint32_t family_index = (last_family_index + j) % static_cast<uint32_t>(num_queues.size());
                if (real_queue_supports[family_index][i]) {
                    allocated_queue_counts[family_index][queue_id] = 1;
                    required_queues[i] = 0;
                    queue_index_in_family[family_index] = (queue_index_in_family[family_index] + 1U) % num_queues[family_index];
                    last_family_index = queue_index_in_family[family_index] == 0 ?
                                        (family_index + 1U) % num_queues.size() : family_index;
                    allocated = true;
                    break;
                }
            }
            if (!allocated) {
                logger().error(
                    "Failed to allocate queue {} due to no family support!",
                    gfx_queues::QUEUE_NAMES[queue_id]
                );
            }
        }
    }

    // 4. Remap queues
    // queue_index_remap[queue_id][index_in_queue_id] = <queue_family_index, queue_index_in_family>
    for (int i = 0; i < gfx_queues::NUM_QUEUES; ++i) {
        out_queue_index_remap[i].clear();
    }
    for (uint32_t family_index = 0; family_index < num_queues.size(); ++family_index) {
        uint32_t queue_index_in_family = 0;
        for (auto&& kv : allocated_queue_counts[family_index]) {
            gfx_queues::QueueId queue_id = kv.first;
            int queue_count_of_queue_id_in_family = kv.second;
            for (int i = 0; i < queue_count_of_queue_id_in_family; ++i) {
                out_queue_index_remap[queue_id].push_back(
                    std::make_pair(family_index, queue_index_in_family)
                );
                queue_index_in_family = (queue_index_in_family + 1U) % num_queues[family_index];
            }
        }
    }
    return succeeded_allocating_all;
}

bool PhysicalDevice::Impl::tryAllocateQueues(
    std::array<int, gfx_queues::NUM_QUEUES> required_queues,
    const std::vector<std::bitset<gfx_queues::NUM_QUEUES>>& real_queue_supports,
    std::vector<std::map<gfx_queues::QueueId, int>>& out_allocated_queue_counts
) {

    out_allocated_queue_counts.clear();
    out_allocated_queue_counts.resize(num_queues.size());

    // 1. Allocate queues if family is large enough to contain all supported queues
    std::vector<int> num_queues_remained = num_queues;
    for (uint32_t family_index = 0; family_index < num_queues.size(); ++family_index) {
        int total_requires = 0;
        for (int i = 0; i < gfx_queues::NUM_QUEUES; ++i) {
            if (real_queue_supports[family_index][i]) {
                total_requires += required_queues[i];
            }
        }

        // This family can contain all queues supported
        if (total_requires < num_queues[family_index]) {
            for (int i = 0; i < gfx_queues::NUM_QUEUES; ++i) {
                auto queue_id = static_cast<gfx_queues::QueueId>(i);
                if (real_queue_supports[family_index][i]) {
                    out_allocated_queue_counts[family_index][queue_id] = required_queues[i];
                    num_queues_remained[family_index] -= required_queues[i];
                    required_queues[i] = 0;
                }
            }
        }
    }

    // 2. Fill queues of each queue_id one by one
    uint32_t family_index = 0;
    for (int i = 0; i < gfx_queues::NUM_QUEUES; ++i) {
        auto queue_id = static_cast<gfx_queues::QueueId>(i);
        bool final_round = false;
        while (required_queues[i] > 0) {
            if (real_queue_supports[family_index][i] && num_queues_remained[family_index] > 0) {
                int allocated = std::min(required_queues[i], num_queues_remained[family_index]);
                out_allocated_queue_counts[family_index][queue_id] += allocated;
                num_queues_remained[family_index] -= allocated;
                required_queues[i] -= allocated;
            } else {
                ++family_index;
                if (family_index >= num_queues.size()) {
                    // If we reached the end of families, try another round.
                    // If we still can't fill queues in this queue_id, we failed.
                    if (final_round) {
                        return false;
                    }
                    final_round = true;
                    family_index = 0;
                }
            }
        }
    }

    return true;
}

bool PhysicalDevice::Impl::checkQueueSupport(
    uint32_t queue_family_index, gfx_queues::QueueId queue_id,
    const std::vector<vk::SurfaceKHR>& surfaces
) {
    if (!queue_supports[queue_family_index][queue_id]) {
        return false;
    }
    if (queue_id == gfx_queues::present) {
        // Check queue family available to all surfaces
        const bool support_all_surfaces = std::all_of(
            surfaces.begin(), surfaces.end(),
            [this, queue_family_index1 = queue_family_index](const vk::SurfaceKHR& surface) {
                if (!vk_physical_device.getSurfaceSupportKHR(queue_family_index1, surface)) {
                    logger().debug("Queue family {} does not support all surfaces.", queue_family_index1);
                    return false;
                }
                return true;
            }
        );
        if (!support_all_surfaces) {
            return false;
        }
    }
    return true;
}

LogicalDevice::LogicalDevice()
    : impl_() {}

void Gfx::createLogicalDevice() {
    logger().info("Creating logical device.");
    if (!physical_device_valid()) {
        logger().error("Failed to create logical device because physical device is invalid.");
        return;
    }

    auto& vk_physical_device = physical_device().impl_
        ->vk_physical_device;
    auto& num_queues_total = physical_device().impl_
        ->num_queues_total;

    // Check and enable available device features
    AvailableVulkanFeatures available_features;
    available_features.getDeviceFeatures(vk_physical_device, num_queues_total);

    logger().info(
        "{} device layers available.", available_features.device_layers
            .size()
    );
    for (auto&& layer : available_features.device_layers) {
        logger().debug(
            " - {} ({}.{}.{}, {}.{}.{}) {}",
            layer.layerName, VK_API_VERSION_MAJOR(layer.specVersion),
            VK_API_VERSION_MINOR(layer.specVersion), VK_API_VERSION_PATCH(layer.specVersion),
            VK_API_VERSION_MAJOR(layer.implementationVersion), VK_API_VERSION_MINOR(layer.implementationVersion),
            VK_API_VERSION_PATCH(layer.implementationVersion), layer.description
        );
    }

    logger().info(
        "{} device extensions available.", available_features.device_extensions
            .size()
    );
    for (auto&& extension : available_features.device_extensions) {
        logger().debug(
            " - {} ({}.{}.{})",
            extension.extensionName, VK_API_VERSION_MAJOR(extension.specVersion),
            VK_API_VERSION_MINOR(extension.specVersion), VK_API_VERSION_PATCH(extension.specVersion)
        );
    }

    logger().info("Device queues:");
    for (uint32_t queue_family_index = 0;
         queue_family_index < physical_device().impl_
             ->num_queues
             .size();
         ++queue_family_index) {

        auto num_queues = physical_device().impl_
            ->num_queues[queue_family_index];
        auto g = physical_device().impl_
            ->checkQueueSupport(queue_family_index, gfx_queues::graphics, {});
        auto p = physical_device().impl_
            ->checkQueueSupport(queue_family_index, gfx_queues::present, {});
        auto t = physical_device().impl_
            ->checkQueueSupport(queue_family_index, gfx_queues::transfer, {});
        auto c = physical_device().impl_
            ->checkQueueSupport(queue_family_index, gfx_queues::compute, {});
        logger().info(
            " - family {}: {}\t{}{}{}{}",
            queue_family_index, num_queues,
            g ? "G" : "", p ? "P" : "", t ? "T" : "", c ? "C" : ""
        );
    }

    VulkanFeatures enabled_features;
    GfxFeaturesManager& gfx_features_manager = GfxFeaturesManager::Get();

    for (auto feature_id : gfx_features_manager.instance_enabled_) {
        VulkanFeatures feature = GetVulkanFeatures(feature_id);
        const bool enabled =
            GfxFeaturesManager::ContainsImpl(feature_id, gfx_features_manager.user_enabled_) ||
                (
                    !GfxFeaturesManager::ContainsImpl(feature_id, gfx_features_manager.user_disabled_) &&
                        GfxFeaturesManager::ContainsImpl(feature_id, gfx_features_manager.defaults_)
                );

        if (enabled && !available_features.checkDeviceAvailable(feature)) {
            logger().info("Gfx feature {} is not available on device.", gfx_features::FEATURE_NAMES[feature_id]);
        } else if (enabled) {
            GfxFeaturesManager::AddFeatureImpl(feature_id, gfx_features_manager.features_enabled_);
            enabled_features.append(feature);
        }
    }

    logger().info("Enabled device features:");
    for (auto feature_id : gfx_features_manager.features_enabled_) {
        logger().info(" - {}", gfx_features::FEATURE_NAMES[feature_id]);
    }

    logger().info("Enabled device layers:");
    for (auto&& layer_name : enabled_features.device_layers) {
        logger().info(" - {}", layer_name);
    }

    logger().info("Enabled device extensions:");
    for (auto&& extension_name : enabled_features.device_extensions) {
        logger().info(" - {}", extension_name);
    }

    logger().info("Enabled device queues:");
    for (int i = 0; i < gfx_queues::NUM_QUEUES; ++i) {
        int queue_num = enabled_features.device_queues[i];
        if (queue_num > 0) {
            logger().info(" - {}[{}]", gfx_queues::QUEUE_NAMES[i], queue_num);
        }
    }

    std::vector<vk::SurfaceKHR> surfaces;
    for (auto&& window_surface : impl_->window_surfaces_) {
        surfaces.emplace_back(
            *window_surface.surface
                ->impl_
                ->vk_surface
        );
    }

    // Allocate queues.
    // <queue_id, index_in_queue_id> => <queue_family_index, queue_index_in_family>
    std::array<std::vector<std::pair<uint32_t, uint32_t>>, gfx_queues::NUM_QUEUES> queue_index_remap;
    bool succeeded = physical_device().impl_
        ->allocateQueues(
            enabled_features.device_queues, surfaces, queue_index_remap
        );
    // queue_counts[queue_family_index] = num of queues to be allocated in queue family
    std::map<uint32_t, uint32_t> queue_counts;
    for (int i = 0; i < gfx_queues::NUM_QUEUES; ++i) {
        for (auto&& kv : queue_index_remap[i]) {
            uint32_t queue_family_index = kv.first;
            uint32_t queue_index_in_family = kv.second;
            queue_counts[queue_family_index] = std::max(
                queue_counts[queue_family_index], queue_index_in_family + 1U
            );
        }
    }

    logger().info("Creating device queues:");
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    std::vector<std::vector<float>> queue_priorities;
    for (auto&&[queue_family_index, queue_count] : queue_counts) {
        if (queue_count > 0) {
            logger().info(" - family {}: {}", queue_family_index, queue_count);
            vk::DeviceQueueCreateInfo queue_create_info{
                .queueFamilyIndex = queue_family_index,
                .queueCount = queue_count
            };
            queue_priorities.emplace_back(std::vector<float>(queue_count, 1.0f));
            queue_create_info.setQueuePriorities(queue_priorities.back());
            queue_create_infos.emplace_back(queue_create_info);
        }
    }

    if (queue_create_infos.empty()) {
        logger().warn("Not creating logical device because no queues needed.");
        return;
    }

    auto device_create_info = vk::DeviceCreateInfo{}
        .setPEnabledLayerNames(enabled_features.device_layers)
        .setPEnabledExtensionNames(enabled_features.device_extensions)
        .setPEnabledFeatures(&enabled_features.device_features)
        .setQueueCreateInfos(queue_create_infos);

    vk::raii::Device vk_device = vk_physical_device.createDevice(device_create_info);

    logical_device_ = std::make_unique<LogicalDevice>();
    logical_device_->impl_ = std::make_unique<LogicalDevice::Impl>(std::move(vk_device));

    // Get all queues
    logical_device_->impl_
        ->allocated_queues
        .resize(
            physical_device().impl_
                ->num_queues
                .size()
        );
    for (auto&&[queue_family_index, queue_count] : queue_counts) {
        for (uint32_t queue_index_in_family = 0; queue_index_in_family < queue_count; ++queue_index_in_family) {
            auto queue_info = std::make_unique<QueueInfo>();
            queue_info->queue_family_index = queue_family_index;
            queue_info->queue_index_in_family = queue_index_in_family;
            queue_info->vk_queue =
                logical_device_->impl_
                    ->vk_device
                    .getQueue(queue_family_index, queue_index_in_family);

            auto command_pool_create_info = vk::CommandPoolCreateInfo{
                .queueFamilyIndex = queue_family_index
            };
            queue_info->vk_command_pool = logical_device_->impl_
                ->vk_device
                .createCommandPool(command_pool_create_info);

            logical_device_->impl_
                ->allocated_queues[queue_family_index].emplace_back(std::move(queue_info));
        }
    }

    // Get all queue references
    for (int i = 0; i < gfx_queues::NUM_QUEUES; ++i) {
        gfx_queues::QueueId queue_id = static_cast<gfx_queues::QueueId>(i);
        for (auto&& kv : queue_index_remap[i]) {
            uint32_t queue_family_index = kv.first;
            uint32_t queue_index_in_family = kv.second;

            const auto& queue_info =
                *logical_device_->impl_
                    ->allocated_queues[queue_family_index][queue_index_in_family].get();

            auto queue_info_ref = QueueInfoRef{
                .queue_family_index = queue_family_index,
                .queue_index_in_family = queue_index_in_family,
                .vk_device = &logical_device_->impl_
                    ->vk_device,
                .vk_queue = *queue_info.vk_queue,
                .vk_command_pool = *queue_info.vk_command_pool
            };
            logical_device_->impl_
                ->queue_references[queue_id].emplace_back(queue_info_ref);
        }
    }

    logger().info("Logical device created.");

    // Create swapchains for surfaces
    for (auto& window_surface : impl_->window_surfaces_) {
        bool result = createWindowSurfaceResources(window_surface.surface);
        if (!result) {
            logger().error(
                "Failed to create surface resources for window \"{}\".",
                window_surface.surface
                    ->window_title()
            );
        }
    }
}

void Gfx::waitDeviceIdle() {
    if (logical_device_) {
        logical_device_->impl_
            ->vk_device
            .waitIdle();
    }
}

bool GfxFeaturesManager::enableFeature(gfx_features::FeatureId feature) {
    if (!ContainsImpl(feature, instance_enabled_)) {
        return false;
    }
    AddFeatureImpl(feature, user_enabled_);
    RemoveFeatureImpl(feature, user_disabled_);
    return true;
}

bool GfxFeaturesManager::disableFeature(gfx_features::FeatureId feature) {
    RemoveFeatureImpl(feature, user_enabled_);
    AddFeatureImpl(feature, user_disabled_);
    return true;
}

std::vector<gfx_features::FeatureId> GfxFeaturesManager::features_required() const {
    std::vector<gfx_features::FeatureId> result;
    for (auto&& feature_id : defaults_) {
        AddFeatureImpl(feature_id, result);
    }
    for (auto&& feature_id : user_enabled_) {
        AddFeatureImpl(feature_id, result);
    }
    for (auto&& feature_id : user_disabled_) {
        RemoveFeatureImpl(feature_id, result);
    }
    return instance_enabled_;
}

bool GfxFeaturesManager::feature_required(gfx_features::FeatureId feature_id) const {
    if (std::find(user_disabled_.begin(), user_disabled_.end(), feature_id) != user_disabled_.end()) {
        return false;
    } else if (std::find(defaults_.begin(), defaults_.end(), feature_id) != defaults_.end()) {
        return true;
    } else {
        return std::find(user_enabled_.begin(), user_enabled_.end(), feature_id) != user_enabled_.end();
    }
}

std::vector<gfx_features::FeatureId> GfxFeaturesManager::features_enabled() const {
    return features_enabled_;
}

bool GfxFeaturesManager::feature_enabled(gfx_features::FeatureId feature_id) const {
    return std::find(features_enabled_.begin(), features_enabled_.end(), feature_id) != features_enabled_.end();
}

std::array<int, gfx_queues::NUM_QUEUES> GfxFeaturesManager::queues_required() const {
    std::array<int, gfx_queues::NUM_QUEUES> result{};
    GetQueuesImpl(result, features_required());
    return result;
}

std::array<int, gfx_queues::NUM_QUEUES> GfxFeaturesManager::queues_enabled() const {
    std::array<int, gfx_queues::NUM_QUEUES> result{};
    GetQueuesImpl(result, features_enabled());
    return result;
}

void GfxFeaturesManager::AddFeatureImpl(
    gfx_features::FeatureId feature,
    std::vector<gfx_features::FeatureId>& features
) {

    if (std::find(features.begin(), features.end(), feature) == features.end()) {
        features.push_back(feature);
    }
}

void GfxFeaturesManager::RemoveFeatureImpl(
    gfx_features::FeatureId feature,
    std::vector<gfx_features::FeatureId>& features
) {

    features.erase(std::remove(features.begin(), features.end(), feature), features.end());
}

bool GfxFeaturesManager::ContainsImpl(
    gfx_features::FeatureId feature,
    const std::vector<gfx_features::FeatureId>& features
) {

    return std::find(features.begin(), features.end(), feature) != features.end();
}

void GfxFeaturesManager::GetQueuesImpl(
    std::array<int, gfx_queues::NUM_QUEUES>& queues,
    const std::vector<gfx_features::FeatureId>& features
) {

    for (auto&& feature_id : features) {
        auto feature = GetVulkanFeatures(feature_id);
        for (int i = 0; i < gfx_queues::NUM_QUEUES; ++i) {
            queues[i] += feature.device_queues[i];
        }
    }
}

} // namespace wg