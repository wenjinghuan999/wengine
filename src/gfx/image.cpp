#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "common/config.h"
#include "gfx/gfx.h"
#include "gfx/image.h"
#include "common/logger.h"
#include "gfx-private.h"
#include "image-private.h"

namespace {

[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

std::shared_ptr<Image> Image::Load(
    const std::string& filename, gfx_formats::Format image_format,
    bool keep_cpu_data, image_file_formats::ImageFileFormat file_format
) {
    return std::shared_ptr<Image>(new Image(filename, image_format, keep_cpu_data, file_format));
}

Image::Image(const std::string& filename, gfx_formats::Format image_format, bool keep_cpu_data, image_file_formats::ImageFileFormat file_format)
    : GfxMemoryBase(keep_cpu_data), filename_(filename), impl_(std::make_unique<Image::Impl>()) {
    load(filename, image_format, file_format);
}

bool Image::load(const std::string& filename, gfx_formats::Format image_format, image_file_formats::ImageFileFormat file_format) {
    logger().info("Loading image: {}", filename);

    int desired_channels = gfx_formats::GetChannels(image_format);
    int width = 0, height = 0, channels = 0;
    stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &channels, desired_channels);
    size_t data_size = static_cast<size_t>(width * height * desired_channels);

    if (!pixels) {
        logger().error("Unable to load image {}", filename);
        return false;
    }

    raw_data_.resize(data_size);
    std::memcpy(raw_data_.data(), pixels, data_size);

    filename_ = filename;
    file_format_ = file_format;
    image_format_ = image_format;
    width_ = width;
    height_ = height;
    mip_levels_ = 1;

    has_cpu_data_ = true;
    has_gpu_data_ = false;

    stbi_image_free(pixels);
    return true;
}

void Image::clearCpuData() {
    std::vector<uint8_t> empty_data;
    raw_data_.swap(empty_data);
}

GfxMemoryBase::Impl* Image::getImpl() {
    return impl_.get();
}

void Gfx::createImageResources(const std::shared_ptr<Image>& image) {
    impl_->createReferenceImageResources(image, image);
    if (image->has_cpu_data()) {
        commitImage(image);
    }
}

void Gfx::Impl::transitionImageLayout(
    ImageResources* image_resources, vk::ImageLayout new_layout, const QueueInfoRef& new_queue
) {
    if (!image_resources) {
        logger().error("Cannot transition image layout because image resources are not available.");
        return;
    }

    transitionImageLayout(
        image_resources, image_resources->image_layout, new_layout, image_resources->queue, new_queue, 0, image_resources->mip_levels
    );

    image_resources->image_layout = new_layout;
    image_resources->queue = new_queue;
}

void Gfx::Impl::transitionImageLayout(
    ImageResources* image_resources,
    vk::ImageLayout old_layout, vk::ImageLayout new_layout,
    const QueueInfoRef& old_queue, const QueueInfoRef& new_queue,
    uint32_t base_mip, uint32_t mip_count
) {
    if (!image_resources) {
        logger().error("Cannot transition image layout because image resources are not available.");
        return;
    }
    if (old_layout == new_layout && old_queue.queue_family_index == new_queue.queue_family_index) {
        return;
    }

    auto barrier = vk::ImageMemoryBarrier{
        .oldLayout           = old_layout,
        .newLayout           = new_layout,
        .srcQueueFamilyIndex = old_queue.queue_family_index,
        .dstQueueFamilyIndex = new_queue.queue_family_index,
        .image               = *image_resources->image,
        .subresourceRange    = {
            .aspectMask      = vk::ImageAspectFlagBits::eColor, // will fix below
            .baseMipLevel    = base_mip,
            .levelCount      = mip_count,
            .baseArrayLayer  = 0,
            .layerCount      = 1
        }
    };

    if (new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        const bool format_has_stencil = [](vk::Format format) {
            return format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD16UnormS8Uint;
        }(image_resources->format);
        if (format_has_stencil) {
            barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    }

    vk::PipelineStageFlagBits src_stage;
    vk::PipelineStageFlagBits dst_stage;
    if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
        dst_stage = vk::PipelineStageFlagBits::eTransfer;
    } else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        src_stage = vk::PipelineStageFlagBits::eTransfer;
        dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (old_layout == vk::ImageLayout::eTransferSrcOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        src_stage = vk::PipelineStageFlagBits::eTransfer;
        dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eTransferSrcOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        src_stage = vk::PipelineStageFlagBits::eTransfer;
        dst_stage = vk::PipelineStageFlagBits::eTransfer;
    } else if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
        dst_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else {
        logger().error("Unsupported image layout transition: {} => {}", vk::to_string(old_layout), vk::to_string(new_layout));
        return;
    }

    if (old_queue.queue_family_index == new_queue.queue_family_index) {
        singleTimeCommand(
            old_queue,
            [&barrier, src_stage, dst_stage](vk::CommandBuffer& command_buffer) {
                command_buffer.pipelineBarrier(
                    src_stage, dst_stage, {},
                    {}, {}, { barrier }
                );
            }
        );
    } else {
        // Ownership transfer
        // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/vkspec.html#synchronization-queue-transfers
        std::vector<vk::Fence> wait_fences;
        for (uint32_t i = 0; i < mip_count; ++i) {
            wait_fences.push_back(*image_resources->ownership_transfer_fences[base_mip + i]);
        }
        auto result = gfx->logical_device_->impl_->vk_device.waitForFences(wait_fences, true, UINT64_MAX);
        if (result != vk::Result::eSuccess) {
            logger().error("Error waiting for fence before ownership transfer: {}. ", vk::to_string(result));
        }
        vk::Semaphore semaphore = *image_resources->ownership_transfer_semaphores[base_mip];
        singleTimeCommand(
            old_queue,
            [&barrier, src_stage, dst_stage](vk::CommandBuffer& command_buffer) {
                command_buffer.pipelineBarrier(
                    src_stage, dst_stage, {},
                    {}, {}, { barrier }
                );
            },
            {}, {}, { semaphore }
        );
        singleTimeCommand(
            new_queue,
            [&barrier, src_stage, dst_stage](vk::CommandBuffer& command_buffer) {
                command_buffer.pipelineBarrier(
                    src_stage, dst_stage, {},
                    {}, {}, { barrier }
                );
            },
            { semaphore }, { vk::PipelineStageFlagBits::eTransfer }, {}
        );
    }
}

void Gfx::Impl::copyBufferToImage(
    const QueueInfoRef& transfer_queue,
    vk::Buffer src, vk::Image dst, uint32_t width, uint32_t height, vk::ImageLayout image_layout
) {
    auto buffer_image_copy = vk::BufferImageCopy{
        .bufferOffset       = 0,
        .bufferRowLength    = 0,
        .bufferImageHeight  = 0,
        .imageSubresource   = {
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .mipLevel       = 0,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
        .imageOffset        = { 0, 0, 0 },
        .imageExtent        = { width, height, 1 }
    };
    singleTimeCommand(
        transfer_queue,
        [&src, &dst, &image_layout, &buffer_image_copy](vk::CommandBuffer& command_buffer) {
            command_buffer.copyBufferToImage(src, dst, image_layout, { buffer_image_copy });
        }
    );
}

void Gfx::Impl::createImageResources(
    const std::shared_ptr<Image>& image
) {
    createReferenceImageResources(image, image);
}

void Gfx::Impl::createReferenceImageResources(
    const std::shared_ptr<Image>& cpu_image,
    const std::shared_ptr<Image>& gpu_image
) {
    gpu_image->impl_->memory_resources.reset();
    gpu_image->impl_->resources.reset();
    gpu_image->has_gpu_data_ = false;

    if (!gfx->logical_device_) {
        logger().error("Cannot create image resources because logical device is not available.");
        return;
    }
    gfx->waitDeviceIdle();

    if (!cpu_image->has_cpu_data()) {
        logger().warn("Skip creating image resources because image \"{}\" is not loaded.", cpu_image->filename());
        return;
    }
    logger().info("Creating resources for image \"{}\".", cpu_image->filename());

    auto max_mip_levels = static_cast<int>(std::floor(std::log2(std::max(cpu_image->width_, cpu_image->height_)))) + 1;
    const bool need_generate_mipmap = cpu_image->mip_levels_ < max_mip_levels;
    bool can_generate_mipmap = need_generate_mipmap;
    auto format_properties = gfx->physical_device().impl_->vk_physical_device.getFormatProperties(gfx_formats::ToVkFormat(cpu_image->image_format_));
    if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        can_generate_mipmap = false;
    }

    gpu_image->width_ = cpu_image->width_;
    gpu_image->height_ = cpu_image->height_;
    gpu_image->mip_levels_ = cpu_image->mip_levels_;

    auto resources = std::make_unique<ImageResources>();
    auto memory_resources = std::make_unique<GfxMemoryResources>();

    int real_mip_levels = can_generate_mipmap ? max_mip_levels : gpu_image->mip_levels_;
    createImage(
        gpu_image->width_, gpu_image->height_, real_mip_levels, gfx_formats::ToVkFormat(cpu_image->image_format()),
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor,
        *resources, *memory_resources
    );

    gpu_image->impl_->memory_resources = gfx->logical_device_->impl_->memory_resources.store(std::move(memory_resources));
    gpu_image->impl_->resources = gfx->logical_device_->impl_->image_resources.store(std::move(resources));
}

void Gfx::Impl::createImage(
    uint32_t width, uint32_t height, uint32_t mip_levels, vk::Format vk_format,
    vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect,
    ImageResources& out_image_resources, GfxMemoryResources& out_memory_resources
) {
    if (!gfx->logical_device_) {
        logger().error("Cannot create image resources because logical device is not available.");
        return;
    }
    gfx->waitDeviceIdle();

    auto graphics_queue = gfx->logical_device_->impl_->queue_references[gfx_queues::graphics][0];

    out_image_resources.width = width;
    out_image_resources.height = height;
    out_image_resources.mip_levels = mip_levels;
    out_image_resources.image_layout = vk::ImageLayout::eUndefined;
    out_image_resources.format = vk_format;
    out_image_resources.queue = graphics_queue;
    auto queue_family_indices = std::array{ graphics_queue.queue_family_index };

    auto image_create_info = vk::ImageCreateInfo{
        .flags         = {},
        .imageType     = vk::ImageType::e2D,
        .format        = vk_format,
        .extent        = {
            .width     = width,
            .height    = height,
            .depth     = 1
        },
        .mipLevels     = mip_levels,
        .arrayLayers   = 1,
        .samples       = vk::SampleCountFlagBits::e1,
        .tiling        = vk::ImageTiling::eOptimal,
        .usage         = usage,
        .sharingMode   = vk::SharingMode::eExclusive,
        .initialLayout = out_image_resources.image_layout
    }
        .setQueueFamilyIndices(queue_family_indices);

    out_image_resources.image = gfx->logical_device_->impl_->vk_device.createImage(image_create_info);

    if (!createGfxMemory(
        out_image_resources.image.getMemoryRequirements(),
        vk::MemoryPropertyFlagBits::eDeviceLocal, out_memory_resources
    )) {
        return;
    }

    out_image_resources.image.bindMemory(*out_memory_resources.memory, 0);

    auto image_view_create_info = vk::ImageViewCreateInfo{
        .image              = *out_image_resources.image,
        .viewType           = vk::ImageViewType::e2D,
        .format             = vk_format,
        .subresourceRange   = {
            .aspectMask     = aspect,
            .baseMipLevel   = 0,
            .levelCount     = mip_levels,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };
    out_image_resources.image_view = gfx->logical_device_->impl_->vk_device.createImageView(image_view_create_info);

    QueueInfoRef transfer_queue;
    getTransferQueue(transfer_queue);
    if (transfer_queue.queue_family_index != graphics_queue.queue_family_index) {
        for (uint32_t i = 0; i < mip_levels; ++i) {
            out_image_resources.ownership_transfer_semaphores.emplace_back(
                gfx->logical_device_->impl_->vk_device.createSemaphore({})
            );
            out_image_resources.ownership_transfer_fences.emplace_back(
                gfx->logical_device_->impl_->vk_device.createFence(
                    {
                        .flags = { vk::FenceCreateFlagBits::eSignaled }
                    }
                )
            );
        }
    }
}

std::shared_ptr<Sampler> Sampler::Create(std::shared_ptr<Image> image) {
    return std::shared_ptr<Sampler>(new Sampler(std::move(image)));
}

Sampler::Sampler(std::shared_ptr<Image> image)
    : image_(std::move(image)), impl_(std::make_unique<Sampler::Impl>()) {}

void Gfx::createSamplerResources(const std::shared_ptr<Sampler>& sampler) {
    sampler->impl_->resources.reset();

    if (!logical_device_) {
        logger().error("Cannot create sampler resources because logical device is not available.");
        return;
    }
    waitDeviceIdle();
    
    if (!sampler->image_) {
        logger().error("Cannot create sampler resources because image is not available.");
        return;
    }

    auto image_resources = sampler->image_->impl_->resources.data();
    if (!image_resources) {
        logger().error("Cannot create sampler resources because image resources are not available.");
        return;
    }

    auto sampler_resources = std::make_unique<SamplerResources>();
    impl_->createSampler(*image_resources, sampler->config(), *sampler_resources);
    sampler->impl_->resources = logical_device_->impl_->sampler_resources.store(std::move(sampler_resources));
}

void Gfx::Impl::createSampler(const ImageResources& image_resources, SamplerConfig config, SamplerResources& out_sampler_resources) {

    if (!gfx->logical_device_) {
        logger().error("Cannot create image resources because logical device is not available.");
        return;
    }
    gfx->waitDeviceIdle();

    auto device_properties = gfx->physical_device().impl_->vk_physical_device.getProperties();
    float max_anisotropy = 0.f;
    if (GfxFeaturesManager::Get().feature_enabled(gfx_features::sampler_anisotropy)) {
        float engine_max_anisotropy = EngineConfig::Get().get<float>("gfx-max-sampler-anisotropy");
        engine_max_anisotropy = std::min<float>(engine_max_anisotropy, device_properties.limits.maxSamplerAnisotropy);
        max_anisotropy = std::min(config.max_anisotropy, engine_max_anisotropy);
    }
    bool integer_format = gfx_formats::IsIntegerFormat(gfx_formats::FromVkFormat(image_resources.format));

    auto sampler_create_info = vk::SamplerCreateInfo{
        .magFilter = image_sampler::ToVkFilter(config.mag_filter),
        .minFilter = image_sampler::ToVkFilter(config.min_filter),
        .mipmapMode = image_sampler::ToVkMipMapMode(config.mip_map_mode),
        .addressModeU = image_sampler::ToVkAddressMode(config.address_u),
        .addressModeV = image_sampler::ToVkAddressMode(config.address_v),
        .addressModeW = image_sampler::ToVkAddressMode(config.address_w),
        .mipLodBias = config.mip_lod_bias,
        .anisotropyEnable = max_anisotropy > 0.f,
        .maxAnisotropy = max_anisotropy,
        .compareEnable = false,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0,
        .maxLod = static_cast<float>(image_resources.mip_levels) - config.mip_lod_bias,
        .borderColor = image_sampler::ToVkBorderColor(config.border_color, integer_format),
        .unnormalizedCoordinates = false
    };
    out_sampler_resources.sampler = gfx->logical_device_->impl_->vk_device.createSampler(sampler_create_info);
}

void Gfx::commitImage(const std::shared_ptr<Image>& image) {
    commitReferenceImage(image, image);
}

void Gfx::commitReferenceImage(
    const std::shared_ptr<Image>& cpu_image,
    const std::shared_ptr<Image>& gpu_image
) {
    if (!cpu_image->has_cpu_data()) {
        logger().error("Skip creating image resources because has no CPU data.");
        return;
    }
    waitDeviceIdle();

    auto* memory_resources = gpu_image->impl_->memory_resources.data();
    auto* resources = gpu_image->impl_->resources.data();
    if (!memory_resources || !resources) {
        logger().error("Cannot commit image because resources are not available.");
        return;
    }

    if (cpu_image->width_ != resources->width || cpu_image->height_ != resources->height) {
        logger().error("Cannot commit image because cpu image size differs from gpu resources.");
        return;
    }

    QueueInfoRef transfer_queue;
    impl_->getTransferQueue(transfer_queue);

    const size_t data_size = cpu_image->data_size();

    GfxMemoryResources staging_memory_resources;
    GfxBufferResources staging_resources;

    // Allocate stage buffer.
    impl_->createBuffer(
        data_size, vk::BufferUsageFlagBits::eTransferSrc,
        vk::SharingMode::eExclusive, { transfer_queue.queue_family_index },
        staging_resources
    );
    if (!impl_->createGfxMemory(
        staging_resources.buffer.getMemoryRequirements(),
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_memory_resources
    )) {
        return;
    }
    staging_resources.buffer.bindMemory(*staging_memory_resources.memory, 0);

    // Copy data to target buffer or stage buffer.
    void* mapped = staging_memory_resources.memory.mapMemory(0, data_size, {});
    std::memcpy(mapped, cpu_image->data(), data_size);
    staging_memory_resources.memory.unmapMemory();

    // Copy content.
    auto graphics_queue = resources->queue;
    impl_->transitionImageLayout(resources, vk::ImageLayout::eTransferDstOptimal, transfer_queue);
    impl_->copyBufferToImage(
        transfer_queue, *staging_resources.buffer, *resources->image, resources->width, resources->height, resources->image_layout
    );

    // Generate mipmaps
    if (cpu_image->mip_levels_ < static_cast<int>(resources->mip_levels)) {
        impl_->singleTimeCommand(
            transfer_queue,
            [resources, &cpu_image](vk::CommandBuffer& command_buffer) {
                auto barrier = vk::ImageMemoryBarrier{
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image               = *resources->image,
                    .subresourceRange    = {
                        .aspectMask      = vk::ImageAspectFlagBits::eColor,
                        .levelCount      = 1,
                        .baseArrayLayer  = 0,
                        .layerCount      = 1
                    }
                };

                int32_t mip_width = cpu_image->width_;
                int32_t mip_height = cpu_image->height_;
                for (uint32_t i = 1U; i < resources->mip_levels; ++i) {
                    // wait for transfer (as destination) to finish, then transition to transfer source
                    barrier.subresourceRange.baseMipLevel = i - 1U;
                    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
                    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
                    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                    barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
                    command_buffer.pipelineBarrier(
                        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {},
                        {}, {}, { barrier }
                    );

                    auto src_offsets = std::array{
                        vk::Offset3D{ 0, 0, 0 },
                        vk::Offset3D{ mip_width, mip_height, 1 }
                    };
                    mip_width = mip_width > 1 ? mip_width / 2 : 1;
                    mip_height = mip_height > 1 ? mip_height / 2 : 1;
                    auto dst_offsets = std::array{
                        vk::Offset3D{ 0, 0, 0 },
                        vk::Offset3D{ mip_width, mip_height, 1 }
                    };

                    auto blit = vk::ImageBlit{
                        .srcSubresource = {
                            .aspectMask = vk::ImageAspectFlagBits::eColor,
                            .mipLevel = static_cast<uint32_t>(i - 1),
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                        },
                        .srcOffsets = src_offsets,
                        .dstSubresource = {
                            .aspectMask = vk::ImageAspectFlagBits::eColor,
                            .mipLevel = static_cast<uint32_t>(i),
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                        },
                        .dstOffsets = dst_offsets
                    };

                    command_buffer.blitImage(
                        *resources->image, vk::ImageLayout::eTransferSrcOptimal,
                        *resources->image, vk::ImageLayout::eTransferDstOptimal,
                        { blit }, vk::Filter::eLinear
                    );
                }
            }
        );

        for (uint32_t i = 1; i < resources->mip_levels; ++i) {
            impl_->transitionImageLayout(
                resources, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, transfer_queue, graphics_queue, i - 1U, 1U
            );
        }
        impl_->transitionImageLayout(
            resources, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, transfer_queue, graphics_queue,
            resources->mip_levels - 1U, 1U
        );
        resources->image_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        resources->queue = graphics_queue;
    } else {
        resources->mip_levels = 1;
        impl_->transitionImageLayout(resources, vk::ImageLayout::eShaderReadOnlyOptimal, graphics_queue);
    }

    gpu_image->has_gpu_data_ = true;
    if (!cpu_image->keep_cpu_data_) {
        cpu_image->clearCpuData();
    }
}

} // namespace wg
