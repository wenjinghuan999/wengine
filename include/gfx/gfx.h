#pragma once

#include "common/common.h"
#include "common/singleton.h"
#include "platform/platform.h"

#include <memory>
#include <string>
#include <vector>

namespace wg {

class Surface;
class PhysicalDevice;
class LogicalDevice;
class GfxFeatures;

class Gfx : public IMovable {
public:
    Gfx(const class App& app);
    ~Gfx();
    bool valid() const;

    // Surface
    std::shared_ptr<Surface> createSurface(std::shared_ptr<Window> window);

    // Physical device
    void updatePhysicalDevices();
    int getNumPhysicalDevices() const { return static_cast<int>(physical_devices_.size()); }
    const PhysicalDevice& getPhysicalDevice(int index) const { return *physical_devices_[index]; }
    void selectPhysicalDevice(int index);
    void selectBestPhysicalDevice(int hint_index = 0);
    const PhysicalDevice& physical_device() const { return *physical_devices_[current_physical_device_index_]; }
    bool physical_device_valid() const;

    // Logical device
    void createLogicalDevice();
protected:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::vector<std::shared_ptr<Surface>> surfaces_;
    std::vector<std::unique_ptr<PhysicalDevice>> physical_devices_;
    int current_physical_device_index_{ -1 };
    std::unique_ptr<LogicalDevice> logical_device_;
};

class Surface : public IMovable {
public:
    Surface(std::shared_ptr<Window> window);
    ~Surface() = default;
protected:
    std::shared_ptr<Window> window_;
protected:
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class PhysicalDevice : public IMovable {
public:
    PhysicalDevice(const std::string& name);
    ~PhysicalDevice() = default;
    const std::string& name() const { return name_; };
protected:
    std::string name_;
protected:
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class LogicalDevice : public IMovable {
public:
    LogicalDevice();
    ~LogicalDevice() = default;
protected:
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

namespace gfx_features {
    enum FeatureId {
        // User controlled features
        window_surface,
        // Engine controlled features
        _must_enable_if_valid, NUM_FEATURES = _must_enable_if_valid,
        _debug_utils,
        _NUM_FEATURES
    };
    extern const char* const FEATURE_NAMES[_NUM_FEATURES];
}

namespace gfx_queues {
    enum QueueId {
        graphics,
        present,
        transfer,
        compute,
        NUM_QUEUES
    };
    extern const char* const QUEUE_NAMES[NUM_QUEUES];
}

class GfxFeaturesManager : public Singleton<GfxFeaturesManager> {
    friend class Singleton<GfxFeaturesManager>;
    friend class Gfx;
    friend class LogicalDevice;
public:
    // Enable device feature
    bool enableFeature(gfx_features::FeatureId feature);
    // Disable device feature
    bool disableFeature(gfx_features::FeatureId feature);
    // Features required
    std::vector<gfx_features::FeatureId> features_required() const;
    // Features enabled
    std::vector<gfx_features::FeatureId> features_enabled() const;
    // Queues required
    std::array<int, gfx_queues::NUM_QUEUES> queues_required() const;
    // Queues enabled
    std::array<int, gfx_queues::NUM_QUEUES> queues_enabled() const;
protected:
    static void _AddFeatureImpl(gfx_features::FeatureId feature, 
        std::vector<gfx_features::FeatureId>& features);
    static void _RemoveFeatureImpl(gfx_features::FeatureId feature, 
        std::vector<gfx_features::FeatureId>& features);
    static bool _Contains(gfx_features::FeatureId feature, 
        const std::vector<gfx_features::FeatureId>& features);
    static void _GetQueuesImpl(std::array<int, gfx_queues::NUM_QUEUES>& queues,
        const std::vector<gfx_features::FeatureId>& features);
protected:
    // Features required by engine (initialized by Gfx)
    std::vector<gfx_features::FeatureId> required_;
    // Features might be enabled by user so we should enable on instance (initialized by Gfx)
    std::vector<gfx_features::FeatureId> instance_enabled_;
    // Features enable by default (initialized by Gfx)
    std::vector<gfx_features::FeatureId> defaults_;
    // Features enabled by user (set after Gfx initialization)
    std::vector<gfx_features::FeatureId> user_enabled_;
    // Features disabled by user (set after Gfx initialization)
    std::vector<gfx_features::FeatureId> user_disabled_;
    // Features currently enabled (initialized by device)
    std::vector<gfx_features::FeatureId> features_enabled_;
    // Queues currently enabled (initialized by device)
    std::vector<gfx_queues::QueueId> queues_enabled_;
};

}