#include <fstream>

#include "gfx/gfx.h"
#include "gfx/shader.h"
#include "common/logger.h"
#include "gfx-private.h"
#include "shader-private.h"

namespace {
    
[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

std::shared_ptr<Shader> Shader::Load(
    const std::string& filename, shader_stages::ShaderStage stage, const std::string& entry) {
    return std::shared_ptr<Shader>(new Shader(filename, stage, entry));
}

Shader::Shader(const std::string& filename, shader_stages::ShaderStage stage, const std::string& entry)
    : filename_(filename), entry_(entry), stage_(stage), impl_(std::make_unique<Shader::Impl>()) {
    loadStatic(filename, entry);
}

bool Shader::loaded() const {
    return !impl_->raw_data.empty();
}

bool Shader::valid() const {
    return static_cast<bool>(impl_->resources);
}

} // namespace wg

namespace {
    
bool LoadStaticShaderFile(const std::string& filename, std::vector<uint32_t>& out_data) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        logger().error("Error loading static shader file: {}", filename);
        return false;
    }

    size_t file_size = (size_t) file.tellg();
    out_data.resize(file_size / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(out_data.data()), file_size);

    return true;
}

} // unnamed namespace

namespace wg {

bool Shader::loadStatic(const std::string& filename, const std::string& entry) {
    logger().info("Loading static shader file: {}", filename);
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        logger().error("Error loading static shader file: {}", filename);
        impl_->raw_data.clear();
        return false;
    }
    filename_ = filename;
    entry_ = entry;

    size_t file_size = (size_t) file.tellg();
    impl_->raw_data.resize((file_size + sizeof(uint32_t) - 1) / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(impl_->raw_data.data()), file_size);

    return true;
}

void Gfx::createShaderResources(const std::shared_ptr<Shader>& shader) {
    
    shader->impl_->resources.reset();

    if (!logical_device_) {
        logger().error("Cannot create shader resources because logical device is not available.");
    }
    waitDeviceIdle();

    if (!shader->loaded()) {
        logger().warn("Skip creating shader resources because shader \"{}\" is not loaded.",
            shader->filename());
    }
    logger().info("Creating resources for shader \"{}\".", shader->filename());

    auto shader_resources = std::make_unique<ShaderResources>();

    auto shader_module_create_info = vk::ShaderModuleCreateInfo{}
        .setCode(shader->impl_->raw_data);
    shader_resources->shader_module = logical_device_->impl_->vk_device.createShaderModule(shader_module_create_info);

    vk::ShaderStageFlagBits vk_stage = GetShaderStageFlag(shader->stage());
    if (vk_stage == vk::ShaderStageFlagBits{}) {
        logger().warn("Shader stage is empty: \"{}\"", shader->filename());
    }

    shader->impl_->shader_stage_create_info = vk::PipelineShaderStageCreateInfo{
        .stage = vk_stage,
        .module = *shader_resources->shader_module,
        .pName = shader->entry().c_str()
    };

    shader->impl_->resources = logical_device_->impl_->shader_resources.store(std::move(shader_resources));
}

} // namespace wg