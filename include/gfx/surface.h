#pragma once

#include <memory>

#include "common/common.h"
#include "platform/platform.h"

namespace wg {

class Surface : public IMovable {
public:
    explicit Surface(const std::weak_ptr<Window>& window);
    ~Surface() override = default;
protected:
    std::weak_ptr<Window> window_;
protected:
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}