#pragma once

#include "glm/glm.hpp"

#include "common/common.h"

#include <memory>

namespace wg {

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};

class VertexBuffer : public std::enable_shared_from_this<VertexBuffer> {
public:
    static std::shared_ptr<VertexBuffer> CreateFromVertexArray(std::vector<Vertex> vertices, bool keep_cpu_data = false);
    ~VertexBuffer() = default;
public:
    bool has_cpu_data() const { return has_cpu_data_; }
    bool has_gpu_data() const { return has_gpu_data_; }
    void setVertexArray(std::vector<Vertex> vertices);
protected:
    explicit VertexBuffer(bool keep_cpu_data);
    std::vector<Vertex> vertices_;
    bool keep_cpu_data_;
    bool has_cpu_data_{false};
    bool has_gpu_data_{false};
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}