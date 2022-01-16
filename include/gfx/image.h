﻿#pragma once

#include "common/common.h"
#include "gfx/gfx-constants.h"

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

class Image : public GfxMemoryBase, public std::enable_shared_from_this<Image> {
public:
    static std::shared_ptr<Image> Load(
        const std::string& filename, bool keep_cpu_data = false,
        image_file_formats::ImageFileFormat file_format = image_file_formats::none
    );
    ~Image() override = default;

    const void* data() const override { return raw_data_.data(); }
    size_t data_size() const override { return raw_data_.size(); }

    [[nodiscard]] const std::string& filename() const { return filename_; }
    [[nodiscard]] image_file_formats::ImageFileFormat file_format() const { return file_format_; }
    [[nodiscard]] Size2D size() const { return { width_, height_ }; }

    [[nodiscard]] bool loaded() const;
    bool load(const std::string& filename, image_file_formats::ImageFileFormat file_format = image_file_formats::none);

protected:
    std::string filename_;
    image_file_formats::ImageFileFormat file_format_;
    int width_{ 0 };
    int height_{ 0 };
    std::vector<uint8_t> raw_data_;
    bool keep_cpu_data_{ false };
    bool has_cpu_data_{ false };
    bool has_gpu_data_{ false };

protected:
    friend class Gfx;
    Image(const std::string& filename, bool keep_cpu_data, image_file_formats::ImageFileFormat file_format);
    void clearCpuData() override;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    GfxMemoryBase::Impl* getImpl() override;
};

}
