#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common/common.h"
#include "common/math.h"
#include "gfx/gfx-constants.h"
#include "gfx/image.h"

namespace wg {

class TextureRenderData : public IRenderData, public std::enable_shared_from_this<TextureRenderData> {
public:
    ~TextureRenderData() override = default;
    void createGfxResources(class Gfx& gfx) override;

public:
    std::shared_ptr<Image> image;

protected:
    friend class Texture;
    TextureRenderData() = default;
};

class Texture : public IGfxObject, public std::enable_shared_from_this<Texture> {
public:
    ~Texture() override = default;

    static std::shared_ptr<Texture> Load(
        const std::string& filename, gfx_formats::Format image_format = gfx_formats::R8G8B8A8Srgb,
        image_file_formats::ImageFileFormat file_format = image_file_formats::none
    );

    [[nodiscard]] std::string filename() const { return image_ ? image_->filename() : ""; }
    [[nodiscard]] image_file_formats::ImageFileFormat file_format() const { return image_ ? image_->file_format() : image_file_formats::none; }
    [[nodiscard]] Size2D size() const { return image_ ? image_->size() : Size2D{ 0, 0 }; }
    [[nodiscard]] gfx_formats::Format image_format() const { return image_ ? image_->image_format() : gfx_formats::none; }
    [[nodiscard]] const std::shared_ptr<Image>& image() const { return image_; }

    std::shared_ptr<IRenderData> createRenderData() override;
    const std::shared_ptr<TextureRenderData>& render_data() const { return render_data_; }

protected:
    std::shared_ptr<Image> image_;
    std::shared_ptr<TextureRenderData> render_data_;

protected:
    explicit Texture(
        const std::string& filename, gfx_formats::Format image_format, image_file_formats::ImageFileFormat file_format
    );
};

}
