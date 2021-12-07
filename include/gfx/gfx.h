#pragma once

#include "common/common.h"

#include <memory>

namespace wg {

class Gfx : public IMovable {
public:
    Gfx(const class App& app);
    ~Gfx();
protected:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}