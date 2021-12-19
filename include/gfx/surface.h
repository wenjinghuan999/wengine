#pragma once

#include <memory>

#include "common/common.h"
#include "platform/platform.h"

namespace wg {

class Surface : public std::enable_shared_from_this<Surface> {
public:
    static std::shared_ptr<Surface> Create(const std::shared_ptr<Window>& window);
    std::shared_ptr<Window> window() const { return window_.lock(); }
    ~Surface() = default;
protected:
    std::weak_ptr<Window> window_;
protected:
    explicit Surface(const std::shared_ptr<Window>& window);
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}