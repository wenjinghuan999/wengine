#include "gfx/gfx.h"
#include "engine/texture.h"
#include "common/logger.h"

namespace {

[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

void TextureRenderData::createGfxResources(Gfx& gfx) {
    gfx.createImageResources(image);
}

std::shared_ptr<Texture> Texture::Load(
    const std::string& filename, gfx_formats::Format image_format, image_file_formats::ImageFileFormat file_format
) {
    return std::shared_ptr<Texture>(new Texture(filename, image_format, file_format));
}

Texture::Texture(const std::string& filename, gfx_formats::Format image_format, image_file_formats::ImageFileFormat file_format) {
    image_ = Image::Load(filename, image_format, true, file_format);
}

std::shared_ptr<IRenderData> Texture::createRenderData() {
    render_data_ = std::shared_ptr<TextureRenderData>(new TextureRenderData());
    render_data_->image = image_;
    return render_data_;
}

} // namespace wg
