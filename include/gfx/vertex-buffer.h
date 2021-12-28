#pragma once

#include "glm/glm.hpp"

#include "common/common.h"

#include <cstddef>
#include <memory>

namespace wg {

namespace vertex_attributes {
    enum VertexAttribute{
        none     = 0,
        position = 1,
        color    = 2,
    };
}

struct VertexBufferDescription {
    vertex_attributes::VertexAttribute attribute;
    gfx_formats::Format format;
    uint32_t stride;
    uint32_t offset;
};

struct SimpleVertex {
    glm::vec3 position;
    glm::vec3 color;

    static std::vector<VertexBufferDescription> Descriptions() {
        return {
            { vertex_attributes::position, gfx_formats::R32G32B32Sfloat, sizeof(SimpleVertex), offsetof(SimpleVertex, position) },
            { vertex_attributes::color, gfx_formats::R32G32B32Sfloat, sizeof(SimpleVertex), offsetof(SimpleVertex, color) },
        };
    }
};

template <typename T>
class is_vertex
{
    template<typename U>
    static constexpr auto test(U*)
        -> typename std::is_same<decltype(U::Descriptions()), std::vector<VertexBufferDescription>>::type;

    template<typename>
    static constexpr std::false_type test(...);

    typedef decltype(test<T>(0)) type;
public:
    static constexpr bool value = type::value;
};

class VertexBufferBase {
public:
    ~VertexBufferBase();
    bool has_cpu_data() const { return has_cpu_data_; }
    bool has_gpu_data() const { return has_gpu_data_; }
    virtual std::vector<VertexBufferDescription> descriptions() const = 0;
    virtual size_t data_size() const = 0;
    virtual const void* data() const = 0;
protected:
    bool keep_cpu_data_;
    bool has_cpu_data_{false};
    bool has_gpu_data_{false};
protected:
    explicit VertexBufferBase(bool keep_cpu_data);
    virtual void clearCpuData() = 0;
protected:
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

template<typename VertexType, typename = std::enable_if_t<is_vertex<VertexType>::value>>
class VertexBuffer : public VertexBufferBase, public std::enable_shared_from_this<VertexBuffer<VertexType>> {
public:
    static std::shared_ptr<VertexBuffer> CreateFromVertexArray(std::vector<VertexType> vertices, bool keep_cpu_data = false) {
        auto vertex_buffer = std::shared_ptr<VertexBuffer>(new VertexBuffer(keep_cpu_data));
        vertex_buffer->setVertexArray(std::move(vertices));
        return vertex_buffer;
    }
    void setVertexArray(std::vector<VertexType> vertices) {
        vertices_ = std::move(vertices);
        has_cpu_data_ = true;
        has_gpu_data_ = false;
    }
    virtual std::vector<VertexBufferDescription> descriptions() const override {
        return VertexType::Descriptions();
    }
    virtual size_t data_size() const override { return vertices_.size() * sizeof(VertexType); }
    virtual const void* data() const override { return vertices_.data(); }
protected:
    std::vector<VertexType> vertices_;
protected:
    explicit VertexBuffer(bool keep_cpu_data) : VertexBufferBase(keep_cpu_data) {}
    virtual void clearCpuData() override {
        std::vector<VertexType> empty_vertices;
        std::swap(vertices_, empty_vertices);
        has_cpu_data_ = false;
    }
};

}