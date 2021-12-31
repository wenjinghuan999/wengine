#include <array>

#include "gfx/gfx.h"
#include "gfx/surface.h"
#include "common/logger.h"
#include "platform/inc/window-private.h"
#include "gfx-private.h"
#include "gfx-constants-private.h"
#include "surface-private.h"

namespace {
    
[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

std::map<GLFWwindow*, std::function<void()>> Surface::Impl::glfw_window_to_resized_func_map;

void Surface::Impl::SetFrameBufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto it = glfw_window_to_resized_func_map.find(window);
    if (it != glfw_window_to_resized_func_map.end()) {
        it->second();
    }
}

std::shared_ptr<Surface> Surface::Create(const std::shared_ptr<Window>& window) {
    return std::shared_ptr<Surface>(new Surface(window));
}

Surface::Surface(const std::shared_ptr<Window>& window)
    : window_(window), impl_(std::make_unique<Surface::Impl>()) {}

Surface::~Surface() {
    if (auto window = window_.lock()) {
        Surface::Impl::glfw_window_to_resized_func_map.erase(window->impl_->glfw_window);
    }
}

Extent2D Surface::extent() const {
    if (auto* resources = impl_->resources.get()) {
        return Extent2D(resources->vk_extent.width, resources->vk_extent.height);
    }
    return {};
}

gfx_formats::Format Surface::format() const {
    if (auto* resources = impl_->resources.get()) {
        return gfx_formats::FromVkFormat(resources->vk_format.format);
    }
    return {};
}

void Gfx::createWindowSurface(const std::shared_ptr<Window>& window) {
    auto surface = Surface::Create(window);

    auto& gfxFeaturesManager = GfxFeaturesManager::Get();
    if (!gfxFeaturesManager.enableFeature(gfx_features::window_surface)) {
        spdlog::warn("Failed to enable feature {}. Surface creation may fail.", 
            gfx_features::FEATURE_NAMES[gfx_features::window_surface]);
    }

    VkSurfaceKHR vk_surface;
    VkResult err = glfwCreateWindowSurface(*impl_->instance, window->impl_->glfw_window, nullptr, &vk_surface);
    if (err != VK_SUCCESS) {
        spdlog::error("Failed to create surface for window \"{}\". Error code {}.", window->title(), err);
        return;
    }
    
    surface->impl_->vk_surface = vk::raii::SurfaceKHR(impl_->instance, vk_surface);
    auto window_surface = std::make_unique<WindowSurfaceResources>();
    window_surface->surface = surface;
    window->impl_->surface_handle = impl_->window_surfaces_.store(std::move(window_surface));

    glfwSetFramebufferSizeCallback(window->impl_->glfw_window, Surface::Impl::SetFrameBufferSizeCallback);
    Surface::Impl::glfw_window_to_resized_func_map[window->impl_->glfw_window] = 
        [weak_surface=std::weak_ptr<Surface>(surface)]() {
        if (auto s = weak_surface.lock()) {
            s->resized_ = true;
        }
    };

    spdlog::info("Created surface for window \"{}\".", window->title(), err);
}

std::shared_ptr<Surface> Gfx::getWindowSurface(const std::shared_ptr<Window>& window) const {
    if (!window || !window->impl_->surface_handle || !logical_device_) {
        return {};
    }
    auto& window_surface = impl_->window_surfaces_.get(window->impl_->surface_handle);
    return window_surface.surface;
}

} // namespace wg

namespace {

[[nodiscard]] vk::raii::SwapchainKHR CreateSwapchainForSurface(
    const vk::raii::PhysicalDevice& physical_device, const vk::raii::Device& device, const vk::raii::SurfaceKHR& surface, const wg::Window& window,
    uint32_t graphics_family_index, uint32_t present_family_index, vk::SwapchainKHR old_swapchain, vk::SurfaceFormatKHR& out_format, vk::Extent2D& out_extent
) {
    out_format = [&physical_device, &surface]() {
        auto available_formats = physical_device.getSurfaceFormatsKHR(*surface);
        for (auto&& available_format : available_formats) {
            if (available_format.format == vk::Format::eR8G8B8A8Srgb && available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return available_format;
            } 
        }
        return available_formats[0];
    }();
    auto present_mode = [&physical_device, &surface]() {
        auto available_modes = physical_device.getSurfacePresentModesKHR(*surface);
        for (auto&& available_mode : available_modes) {
            if (available_mode == vk::PresentModeKHR::eMailbox) {
                return available_mode;
            }
        }        
        for (auto&& available_mode : available_modes) {
            if (available_mode == vk::PresentModeKHR::eFifo) {
                return available_mode;
            }
        }
        return available_modes[0];
    }();
    auto capabilities = physical_device.getSurfaceCapabilitiesKHR(*surface);
    out_extent = [&capabilities, &window]() {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }
        auto [width, height] = window.extent();

        vk::Extent2D extent = {
            .width  = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height)
        };

        extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return extent;
    }();

    if (out_extent.width == 0 || out_extent.height == 0) {
        return {nullptr};
    }

    uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }

    std::array<uint32_t, 2> queue_family_indices{ graphics_family_index, present_family_index };

    vk::SwapchainCreateInfoKHR swapchain_create_info{
        .surface          = *surface,
        .minImageCount    = image_count,
        .imageFormat      = out_format.format,
        .imageColorSpace  = out_format.colorSpace,
        .imageExtent      = out_extent,
        .imageArrayLayers = 1,
        .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
        .preTransform     = capabilities.currentTransform,
        .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode      = present_mode,
        .clipped          = true,
        .oldSwapchain     = old_swapchain
    };

    if (graphics_family_index == present_family_index) {
        swapchain_create_info.setImageSharingMode(vk::SharingMode::eExclusive);
    } else {
        swapchain_create_info.setImageSharingMode(vk::SharingMode::eConcurrent);
        swapchain_create_info.setQueueFamilyIndices(queue_family_indices);
    }
    return device.createSwapchainKHR(swapchain_create_info);
}

vk::raii::ImageView CreateImageViewForSurface(
    const vk::raii::Device& device, const vk::Image& image,
    const vk::Format& format
) {
    vk::ImageViewCreateInfo image_view_create_info{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .components = {
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity
        },
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    return device.createImageView(image_view_create_info);
}

} // unnamed namespace

namespace wg {

bool Gfx::createWindowSurfaceResources(const std::shared_ptr<Surface>& surface) {

    if (!logical_device_) {
        logger().error("Cannot create window surface resources because logical device is not available.");
    }
    waitDeviceIdle();

    auto window = surface->window_.lock();
    if (!window) {
        logger().info("Skip creating resources for surface because window is no longer available.");
        return false;
    }
    
    if (logical_device_->impl_->queue_references[gfx_queues::graphics].empty() || 
        logical_device_->impl_->queue_references[gfx_queues::present].empty()) {
        logger().info("Skip creating resources for surface of window \"{}\" because no queues available.", window->title());
        return false;
    }
        
    uint32_t graphics_family_index = logical_device_->impl_->queue_references[gfx_queues::graphics][0].queue_family_index;
    uint32_t present_family_index = logical_device_->impl_->queue_references[gfx_queues::present][0].queue_family_index;
    logger().info(" - Creating swapchain: Graphics family: {}, present family: {}.",
        graphics_family_index, present_family_index);
    
    vk::raii::SwapchainKHR old_swapchain = nullptr;
    if (auto* old_resources = surface->impl_->resources.get()) {
        old_swapchain = std::move(old_resources->vk_swapchain);
    }

    auto resources = std::make_unique<SurfaceResources>();
    resources->vk_swapchain = CreateSwapchainForSurface(
        physical_device().impl_->vk_physical_device, logical_device_->impl_->vk_device,
        surface->impl_->vk_surface, *window,
        graphics_family_index, present_family_index,
        *old_swapchain,
        resources->vk_format, resources->vk_extent
    );

    if (!*resources->vk_swapchain) {
        logger().info("Skip creating resources for hidden surface \"{}\".", window->title());
        surface->hidden_ = true;
        return false;
    }

    logger().info("Creating resources for surface of window \"{}\".", window->title());
    surface->impl_->resources.reset();

    auto vk_images = resources->vk_swapchain.getImages();
    resources->vk_images.reserve(vk_images.size());
    for (auto vk_image : vk_images) {
        resources->vk_images.push_back(static_cast<vk::Image>(vk_image));
    }
    logger().info(" - Creating image views: {}.", resources->vk_images.size());
    resources->vk_image_views.reserve(resources->vk_images.size());
    for (auto&& vk_image : resources->vk_images) {
        resources->vk_image_views.emplace_back(CreateImageViewForSurface(
            logical_device_->impl_->vk_device, vk_image, resources->vk_format.format));
    }

    surface->impl_->resources = logical_device_->impl_->surface_resources.store(std::move(resources));
    surface->resized_ = false;
    surface->hidden_ = false;

    return true;
}

} // namespace wg