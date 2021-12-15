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

class Shader : public IMovable {
public:
    explicit Shader();
    explicit Shader(const std::string& filename, shader_stage::ShaderStage stage);
    ~Shader() override = default;
public:
    bool loaded() const;
    bool valid() const;
    bool loadStatic(const std::string& filename, shader_stage::ShaderStage stage);
protected:
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}