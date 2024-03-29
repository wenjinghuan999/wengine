#pragma once

#include "platform/inc/platform.inc"

#include "gfx/gfx.h"

#include "common/owned-resources.h"
#include "gfx/inc/surface-private.h"
#include "gfx/inc/shader-private.h"
#include "gfx/inc/gfx-pipeline-private.h"
#include "gfx/inc/render-target-private.h"
#include "gfx/inc/gfx-buffer-private.h"
#include "gfx/inc/image-private.h"

#include <array>
#include <bitset>
#include <vector>
#include <tuple>

namespace wg {

struct WindowResources {
    std::shared_ptr<Surface> surface;
};

struct Gfx::Impl {
    vk::raii::Context context;
    vk::raii::Instance instance{ nullptr };
#ifndef NDEBUG
    [[maybe_unused]] vk::raii::DebugUtilsMessengerEXT debug_messenger{ nullptr };
#endif
    OwnedResources<WindowResources> window_resources_;
    OwnedResources<WindowSurfaceResources> window_surface_resources_;
    Gfx* gfx;

    void singleTimeCommand(
        const QueueInfoRef& queue, const std::function<void(vk::CommandBuffer&)>& func,
        std::vector<vk::Semaphore> wait_semaphores = {}, std::vector<vk::PipelineStageFlags> wait_stages = {}, 
        std::vector<vk::Semaphore> signal_semaphores = {}
    );

    bool createGfxMemory(
        vk::MemoryRequirements memory_requirements, vk::MemoryPropertyFlags memory_properties,
        GfxMemoryResources& out_resources
    );

    void createBuffer(
        vk::DeviceSize data_size, vk::BufferUsageFlags usage,
        vk::SharingMode sharing_mode, const std::vector<uint32_t>& queue_family_indices,
        GfxBufferResources& out_resources
    );
    void copyBuffer(const QueueInfoRef& transfer_queue, vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
    vk::SharingMode getTransferQueue(QueueInfoRef& out_transfer_queue) const;
    void createBufferResources(
        const std::shared_ptr<GfxBufferBase>& gfx_buffer,
        vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memory_properties
    );
    void createReferenceBufferResources(
        const std::shared_ptr<GfxBufferBase>& cpu_buffer,
        const std::shared_ptr<GfxBufferBase>& gpu_buffer,
        vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memory_properties
    );

    void transitionImageLayout(
        ImageResources* image_resources,
        vk::ImageLayout old_layout, vk::ImageLayout new_layout,
        const QueueInfoRef& old_queue, const QueueInfoRef& new_queue,
        uint32_t base_mip, uint32_t mip_count, uint32_t layer_count
    );
    void transitionImageLayout(
        ImageResources* image_resources, vk::ImageLayout new_layout, const QueueInfoRef& new_queue
    );
    void copyBufferToImage(
        const QueueInfoRef& transfer_queue, vk::Buffer src, vk::Image dst, 
        uint32_t width, uint32_t height, vk::ImageLayout image_layout, uint32_t layer_count
    );
    void createImageResources(const std::shared_ptr<Image>& image);
    void createReferenceImageResources(
        const std::shared_ptr<Image>& cpu_image,
        const std::shared_ptr<Image>& gpu_image
    );
    void createImage(
        uint32_t width, uint32_t height, uint32_t mip_levels, vk::ImageType vk_image_type, vk::ImageViewType vk_view_type,
        vk::SampleCountFlagBits sample_count, vk::Format vk_format, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect,
        ImageResources& out_image_resources, GfxMemoryResources& out_memory_resources
    );
    
    void createSamplerResources(
        const std::shared_ptr<Image>& gpu_image,
        const std::shared_ptr<Sampler>& sampler
    );
    void createSampler(const ImageResources& image_resources, SamplerConfig config, SamplerResources& out_sampler_resources);

    [[nodiscard]] image_sampler::Filter getCompatibleFilter(image_sampler::Filter filter, gfx_formats::Format format) const;
    [[nodiscard]] vk::SampleCountFlagBits getMaxSampleCount(vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect) const;
};

struct PhysicalDevice::Impl {
    vk::raii::PhysicalDevice vk_physical_device{ nullptr };
    // num_queues_total[queue_id] = total num of queues available in all families
    std::array<int, gfx_queues::NUM_QUEUES> num_queues_total{};
    // num_queues[queue_family_index] = num_queues_of_family
    std::vector<int> num_queues{};
    // queue_supports[queue_family_index] = bitset of support of each gfx_queue
    std::vector<std::bitset<gfx_queues::NUM_QUEUES>> queue_supports{};

    explicit Impl(vk::raii::PhysicalDevice vk_physical_device);

    [[nodiscard]] int findMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags property_flags) const;
    bool allocateQueues(
        std::array<int, gfx_queues::NUM_QUEUES> required_queues,
        const std::vector<vk::SurfaceKHR>& surfaces,
        std::array<std::vector<std::pair<uint32_t, uint32_t>>, gfx_queues::NUM_QUEUES>& out_queue_index_remap
    );
    bool tryAllocateQueues(
        std::array<int, gfx_queues::NUM_QUEUES> required_queues,
        const std::vector<std::bitset<gfx_queues::NUM_QUEUES>>& real_queue_supports,
        std::vector<std::map<gfx_queues::QueueId, int>>& out_allocated_queue_counts
    );
    [[nodiscard]] bool checkQueueSupport(
        uint32_t queue_family_index, gfx_queues::QueueId queue_id,
        const std::vector<vk::SurfaceKHR>& surfaces
    ) const;

protected:
    Impl() = default;
};

struct LogicalDevice::Impl {
    vk::raii::Device vk_device;
    // allocated_queues[queue_family_index][] = QueueInfo
    std::vector<std::vector<std::unique_ptr<QueueInfo>>> allocated_queues;
    // queue_references[queue_id][] = QueueInfoRef
    std::array<std::vector<QueueInfoRef>, gfx_queues::NUM_QUEUES> queue_references;

    // resources (which may be accessed by buffer using OwnedResourcesHandle)
    OwnedResources<SurfaceResources> surface_resources;
    OwnedResources<ShaderResources> shader_resources;
    OwnedResources<GfxPipelineResources> gfx_pipeline_resources;
    OwnedResources<RenderTargetResources> render_target_resources;
    OwnedResources<GfxMemoryResources> memory_resources;
    OwnedResources<GfxBufferResources> buffer_resources;
    OwnedResources<ImageResources> image_resources;
    OwnedResources<SamplerResources> sampler_resources;

    explicit Impl(vk::raii::Device vk_device)
        : vk_device(std::move(vk_device)) {}
};

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
    VulkanFeatures& append(const VulkanFeatures& other);
    [[nodiscard]] static VulkanFeatures GetVulkanFeatures(wg::gfx_features::FeatureId feature);
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
    void getInstanceFeatures(const vk::raii::Context& context);
    void getDeviceFeatures(
        const vk::raii::PhysicalDevice& device,
        std::array<int, wg::gfx_queues::NUM_QUEUES> num_queues_total
    );
    [[nodiscard]] bool checkInstanceAvailable(const VulkanFeatures& feature) const;
    [[nodiscard]] bool checkDeviceAvailable(const VulkanFeatures& feature) const;

private:
    static bool CheckLayerContains(const std::vector<vk::LayerProperties>& layers, const char* layer_name);
    static bool CheckExtensionContains(const std::vector<vk::ExtensionProperties>& extensions, const char* extension_name);
};

} // namespace wg