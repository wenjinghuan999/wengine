#include <fstream>

#include "gfx/shader.h"
#include "common/logger.h"

namespace {
    
auto& logger() {
    static auto logger_ = wg::Logger::Get("shader");
    return *logger_;
}

} // unnamed namespace

namespace wg {

struct Shader::Impl {
    std::vector<uint32_t> raw_data;
};

Shader::Shader() : impl_() {}

Shader::Shader(const std::string& filename, shader_stage::ShaderStage stage) : impl_() {
    loadStatic(filename, stage);
}

bool Shader::loaded() const {
    return !impl_->raw_data.empty();
}

bool Shader::valid() const {
    return false;
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

bool Shader::loadStatic(const std::string& filename, shader_stage::ShaderStage stage) {
    logger().info("Loading static shader file: {}", filename);
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        logger().error("Error loading static shader file: {}", filename);
        impl_->raw_data.clear();
        return false;
    }

    size_t file_size = (size_t) file.tellg();
    impl_->raw_data.resize(file_size / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(impl_->raw_data.data()), file_size);

    return true;
}


} // namespace wg