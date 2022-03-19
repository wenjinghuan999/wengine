#pragma once

#include "common/common.h"
#include "gfx/gfx-constants.h"
#include "gfx/gfx-buffer.h"

#include <memory>
#include <string>

namespace wg {

namespace image_file_formats {

enum ImageFileFormat {
    none = 0,
    png,
};

typedef uint32_t ImageFileFormats;

} // namespace image_file_formats

namespace image_sampler {

enum Filter {
    nearest = 0,
    linear = 1,
    cubic = 1000015000
};

enum MipMapMode {
    mipmap_nearest = 0,
    mipmap_linear = 1,
};

enum AddressMode {
    repeat = 0,
    mirrored_repeat = 1,
    clamp_to_edge = 2,
    clamp_to_border = 3,
    mirror_clamp_to_edge = 4
};

enum BorderColor {
    transparent_black,
    opaque_black,
    opaque_white,
    custom,
};

} // namespace image_sampler

class Image : public GfxMemoryBase, public std::enable_shared_from_this<Image> {
public:
    static std::shared_ptr<Image> Load(
        const std::string& filename, gfx_formats::Format image_format = gfx_formats::R8G8B8A8Unorm,
        bool keep_cpu_data = false, image_file_formats::ImageFileFormat file_format = image_file_formats::none
    );
    ~Image() override = default;

    const void* data() const override { return raw_data_.data(); }
    size_t data_size() const override { return raw_data_.size(); }

    [[nodiscard]] const std::string& filename() const { return filename_; }
    [[nodiscard]] image_file_formats::ImageFileFormat file_format() const { return file_format_; }
    [[nodiscard]] Size2D size() const { return { width_, height_ }; }
    [[nodiscard]] int mip_levels() const { return mip_levels_; }
    [[nodiscard]] gfx_formats::Format image_format() const { return image_format_; }

    bool load(
        const std::string& filename, gfx_formats::Format image_format = gfx_formats::R8G8B8A8Unorm,
        image_file_formats::ImageFileFormat file_format = image_file_formats::none
    );

protected:
    std::string filename_;
    image_file_formats::ImageFileFormat file_format_;
    int width_{ 0 };
    int height_{ 0 };
    int mip_levels_{ 1 };
    gfx_formats::Format image_format_{ gfx_formats::none };
    std::vector<uint8_t> raw_data_;

protected:
    friend class Gfx;
    Image(const std::string& filename, gfx_formats::Format image_format, bool keep_cpu_data, image_file_formats::ImageFileFormat file_format);
    void clearCpuData() override;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    GfxMemoryBase::Impl* getImpl() override;
};

struct SamplerConfig {
    image_sampler::Filter mag_filter = image_sampler::linear;
    image_sampler::Filter min_filter = image_sampler::linear;
    image_sampler::MipMapMode mip_map_mode = image_sampler::mipmap_linear;
    image_sampler::AddressMode address_u = image_sampler::repeat;
    image_sampler::AddressMode address_v = image_sampler::repeat;
    image_sampler::AddressMode address_w = image_sampler::repeat;
    float mip_lod_bias = 0.f;
    float max_anisotropy = 8.f;
    image_sampler::BorderColor border_color = image_sampler::opaque_black;
    bool unnormalized_coordinates = false;
};

class Sampler : public std::enable_shared_from_this<Sampler> {
public:
    static std::shared_ptr<Sampler> Create(std::shared_ptr<Image> image);
    ~Sampler() = default;

    SamplerConfig config() const { return config_; }
    void setConfig(SamplerConfig config) { config_ = config; }
    const std::shared_ptr<Image>& image() const { return image_; }

protected:
    std::shared_ptr<Image> image_;
    SamplerConfig config_;

protected:
    friend class Gfx;
    explicit Sampler(std::shared_ptr<Image> image);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace wg