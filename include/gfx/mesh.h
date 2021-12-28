#pragma once

#include "common/common.h"

#include <memory>

namespace wg {

class Mesh : public std::enable_shared_from_this<Mesh> {
public:
    static std::shared_ptr<Mesh> CreateFromVertices();
    ~Mesh() = default;
    [[nodiscard]] const std::string& name() const { return name_; }
protected:
    explicit Mesh(const std::string& name);
    std::string name_;
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}