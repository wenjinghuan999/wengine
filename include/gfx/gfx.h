#pragma once

#include "common/common.h"
#include "common/singleton.h"
#include "platform/platform.h"
#include "gfx/surface.h"
#include "gfx/shader.h"
#include "gfx/render-target.h"
#include "gfx/gfx-pipeline.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace wg {

class PhysicalDevice;
class LogicalDevice;

class Gfx : public std::enable_shared_from_this<Gfx> {
public:
    static std::shared_ptr<Gfx> Create(const App& app);
    ~Gfx();
    [[nodiscard]] bool valid() const;

    // Surface
    void createWindowSurface(const std::shared_ptr<Window>& window);
    std::shared_ptr<Surface> getWindowSurface(const std::shared_ptr<Window>& window) const;

    // Physical device
    void updatePhysicalDevices();
    [[nodiscard]] int getNumPhysicalDevices() const { return static_cast<int>(physical_devices_.size()); }
    [[nodiscard]] const PhysicalDevice& getPhysicalDevice(int index) const { return *physical_devices_[index]; }
    void selectPhysicalDevice(int index);
    void selectBestPhysicalDevice(int hint_index = 0);
    [[nodiscard]] const PhysicalDevice& physical_device() const { return *physical_devices_[current_physical_device_index_]; }
    [[nodiscard]] bool physical_device_valid() const;

    // Logical device
    void createLogicalDevice();

    // Shader
    void createShaderResources(const std::shared_ptr<Shader>& shader);

    // RenderTarget
    std::shared_ptr<RenderTarget> createRenderTarget(const std::shared_ptr<Window>& window) const;

    // GfxPipeline
    void createPipelineResources(const std::shared_ptr<GfxPipeline>& pipeline);
protected:
    explicit Gfx(const App& app);
    // Surface resources
    void createWindowSurfaceResources(const Surface& surface);
protected:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::vector<std::unique_ptr<PhysicalDevice>> physical_devices_;
    int current_physical_device_index_{ -1 };
    std::unique_ptr<LogicalDevice> logical_device_;
};

class PhysicalDevice : public IMovable {
public:
    explicit PhysicalDevice(std::string name);
    ~PhysicalDevice() override = default;
    [[nodiscard]] const std::string& name() const { return name_; };
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
    ~LogicalDevice() override = default;
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
        NUM_FEATURES_TOTAL
    };
    extern const char* const FEATURE_NAMES[NUM_FEATURES_TOTAL];
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
    [[nodiscard]] std::vector<gfx_features::FeatureId> features_required() const;
    // Features enabled
    [[nodiscard]] std::vector<gfx_features::FeatureId> features_enabled() const;
    // Queues required
    [[nodiscard]] std::array<int, gfx_queues::NUM_QUEUES> queues_required() const;
    // Queues enabled
    [[nodiscard]] std::array<int, gfx_queues::NUM_QUEUES> queues_enabled() const;
protected:
    static void AddFeatureImpl(gfx_features::FeatureId feature, 
        std::vector<gfx_features::FeatureId>& features);
    static void RemoveFeatureImpl(gfx_features::FeatureId feature, 
        std::vector<gfx_features::FeatureId>& features);
    static bool ContainsImpl(gfx_features::FeatureId feature, 
        const std::vector<gfx_features::FeatureId>& features);
    static void GetQueuesImpl(std::array<int, gfx_queues::NUM_QUEUES>& queues,
        const std::vector<gfx_features::FeatureId>& features);
protected:
    // Features required by engine (initialized by Gfx)
    std::vector<gfx_features::FeatureId> required_;
    // Features might be enabled by user, so we should enable on instance (initialized by Gfx)
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