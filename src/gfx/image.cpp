#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

void Gfx::Impl::transitionImageLayout(const std::shared_ptr<Image>& image, vk::ImageLayout layout, const QueueInfoRef& queue) {
    auto* resources = image->impl_->resources.data();
    if (!resources) {
        logger().error("Cannot transition image layout because image resources are not available.");
        return;
    }
    if (resources->image_layout == layout) {
        return;
    }

    auto barrier = vk::ImageMemoryBarrier{
        .oldLayout           = resources->image_layout,
        .newLayout           = layout,
        .srcQueueFamilyIndex = resources->queue.queue_family_index,
        .dstQueueFamilyIndex = queue.queue_family_index,
        .image               = *resources->image,
        .subresourceRange    = {
            .aspectMask      = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel    = 0,
            .levelCount      = 1,
            .baseArrayLayer  = 0,
            .layerCount      = 1
        }
    };

    vk::PipelineStageFlagBits src_stage;
    vk::PipelineStageFlagBits dst_stage;
    if (resources->image_layout == vk::ImageLayout::eUndefined && layout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
        dst_stage = vk::PipelineStageFlagBits::eTransfer;
    } else if (resources->image_layout == vk::ImageLayout::eTransferDstOptimal && layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        src_stage = vk::PipelineStageFlagBits::eTransfer;
        dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        logger().error("Unsupported image layout transition: {} => {}", vk::to_string(resources->image_layout), vk::to_string(layout));
        return;
    }

    singleTimeCommand(
        resources->queue,
        [&barrier, src_stage, dst_stage](vk::CommandBuffer& command_buffer) {
            command_buffer.pipelineBarrier(
                src_stage, dst_stage, {},
                {}, {}, { barrier }
            );
        }
    );
    if (resources->queue.queue_family_index != queue.queue_family_index) {
        // Need a barrier on target queue, too.
        singleTimeCommand(
            queue,
            [&barrier, src_stage, dst_stage](vk::CommandBuffer& command_buffer) {
                command_buffer.pipelineBarrier(
                    src_stage, dst_stage, {},
                    {}, {}, { barrier }
                );
            }
        );
    }
    
    resources->image_layout = layout;
    resources->queue = queue;
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
    }
    gfx->waitDeviceIdle();

    if (!cpu_image->has_cpu_data()) {
        logger().warn("Skip creating image resources because image \"{}\" is not loaded.", cpu_image->filename());
        return;
    }
    logger().info("Creating resources for image \"{}\".", cpu_image->filename());

    gpu_image->width_ = cpu_image->width_;
    gpu_image->height_ = cpu_image->height_;

    auto resources = std::make_unique<ImageResources>();
    auto memory_resources = std::make_unique<GfxMemoryResources>();

    auto graphics_queue = gfx->logical_device_->impl_->queue_references[gfx_queues::graphics][0];

    resources->width = static_cast<uint32_t>(cpu_image->width_);
    resources->height = static_cast<uint32_t>(cpu_image->height_);
    resources->image_layout = vk::ImageLayout::eUndefined;
    resources->queue = graphics_queue;
    auto queue_family_indices = std::array{ graphics_queue.queue_family_index };

    auto image_create_info = vk::ImageCreateInfo{
        .flags         = {},
        .imageType     = vk::ImageType::e2D,
        .format        = gfx_formats::ToVkFormat(cpu_image->image_format()),
        .extent        = {
            .width     = resources->width,
            .height    = resources->height,
            .depth     = 1
        },
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = vk::SampleCountFlagBits::e1,
        .tiling        = vk::ImageTiling::eOptimal,
        .usage         = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        .sharingMode   = vk::SharingMode::eExclusive,
        .initialLayout = resources->image_layout
    }
        .setQueueFamilyIndices(queue_family_indices);

    resources->image = gfx->logical_device_->impl_->vk_device.createImage(image_create_info);
    
    if (!createGfxMemory(
        resources->image.getMemoryRequirements(),
        vk::MemoryPropertyFlagBits::eDeviceLocal, *memory_resources
    )) {
        return;
    }

    resources->image.bindMemory(*memory_resources->memory, 0);

    auto image_view_create_info = vk::ImageViewCreateInfo{
        .image              = *resources->image,
        .viewType           = vk::ImageViewType::e2D,
        .format             = gfx_formats::ToVkFormat(cpu_image->image_format()),
        .subresourceRange   = {
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };
    resources->image_view = gfx->logical_device_->impl_->vk_device.createImageView(image_view_create_info);

    auto device_properties = gfx->physical_device().impl_->vk_physical_device.getProperties();

    auto sampler_create_info = vk::SamplerCreateInfo{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0,
        .anisotropyEnable = false,
        .maxAnisotropy = device_properties.limits.maxSamplerAnisotropy,
        .compareEnable = false,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0,
        .maxLod = 0,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = false
    };
    resources->sampler = gfx->logical_device_->impl_->vk_device.createSampler(sampler_create_info);

    gpu_image->impl_->memory_resources = gfx->logical_device_->impl_->memory_resources.store(std::move(memory_resources));
    gpu_image->impl_->resources = gfx->logical_device_->impl_->image_resources.store(std::move(resources));
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
    impl_->transitionImageLayout(gpu_image, vk::ImageLayout::eTransferDstOptimal, transfer_queue);
    impl_->copyBufferToImage(transfer_queue, *staging_resources.buffer, *resources->image, resources->width, resources->height, resources->image_layout);
    impl_->transitionImageLayout(gpu_image, vk::ImageLayout::eShaderReadOnlyOptimal, graphics_queue);

    gpu_image->has_gpu_data_ = true;
    if (!cpu_image->keep_cpu_data_) {
        cpu_image->clearCpuData();
    }

}

} // namespace wg
