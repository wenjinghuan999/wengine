#pragma once

#include "common/common.h"

#include <memory>

namespace wg {

namespace shader_stage {
    enum ShaderStage {
        none = 0,
        vert = 1,
        frag = 2
    };
}

class Shader : public std::enable_shared_from_this<Shader> {
public:
    static std::shared_ptr<Shader> Load(
        const std::string& filename, shader_stage::ShaderStage stage, const std::string& entry = "main");
    ~Shader() = default;
    [[nodiscard]] const std::string& filename() const { return filename_; }
    [[nodiscard]] shader_stage::ShaderStage stage() const { return stage_; }
    [[nodiscard]] const std::string& entry() const { return entry_; }
public:
    [[nodiscard]] bool loaded() const;
    [[nodiscard]] bool valid() const;
    bool loadStatic(const std::string& filename, const std::string& entry = "main");
    void setStage(shader_stage::ShaderStage stage) { stage_ = stage; }
protected:
    explicit Shader(const std::string& filename, shader_stage::ShaderStage stage, const std::string& entry);
    std::string filename_;
    std::string entry_;
    shader_stage::ShaderStage stage_;
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}