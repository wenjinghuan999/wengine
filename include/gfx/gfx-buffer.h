#pragma once

#include "glm/glm.hpp"

#include "common/common.h"

#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

namespace wg {

namespace vertex_attributes {
    enum VertexAttribute{
        none     = 0,
        position = 1,
        color    = 2,
    };
}

namespace index_types {
    enum IndexType{
        index_16,
        index_32
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

template <typename T>
inline constexpr bool is_vertex_v = is_vertex<T>::value;

class GfxBufferBase {
public:
    virtual ~GfxBufferBase();
    bool has_cpu_data() const { return has_cpu_data_; }
    bool has_gpu_data() const { return has_gpu_data_; }
    virtual size_t data_size() const = 0;
    virtual const void* data() const = 0;
protected:
    bool keep_cpu_data_;
    bool has_cpu_data_{false};
    bool has_gpu_data_{false};
protected:
    explicit GfxBufferBase(bool keep_cpu_data);
    virtual void clearCpuData() = 0;
protected:
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class VertexBufferBase : public GfxBufferBase {
public:
    virtual ~VertexBufferBase() override;
    virtual std::vector<VertexBufferDescription> descriptions() const = 0;
protected:
    explicit VertexBufferBase(bool keep_cpu_data);
};

template<typename VertexType, typename = std::enable_if_t<is_vertex<VertexType>::value>>
class VertexBuffer : public VertexBufferBase, public std::enable_shared_from_this<VertexBuffer<VertexType>> {
public:
    static std::shared_ptr<VertexBuffer> CreateFromVertexArray(std::vector<VertexType> vertices, bool keep_cpu_data = false) {
        auto vertex_buffer = std::shared_ptr<VertexBuffer<VertexType>>(new VertexBuffer<VertexType>(keep_cpu_data));
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
    friend class Gfx;
    explicit VertexBuffer(bool keep_cpu_data) : VertexBufferBase(keep_cpu_data) {}
    virtual void clearCpuData() override {
        std::vector<VertexType> empty_vertices;
        std::swap(vertices_, empty_vertices);
        has_cpu_data_ = false;
    }
};

class IndexBuffer : public GfxBufferBase {
public:
    template <typename IndexType, typename = std::enable_if_t<std::is_integral_v<IndexType>>>
    static std::shared_ptr<IndexBuffer> CreateFromIndexArray(index_types::IndexType index_type, const std::vector<IndexType>& indices, bool keep_cpu_data = false) {
        auto index_buffer = std::shared_ptr<IndexBuffer>(new IndexBuffer(index_type, keep_cpu_data));
        index_buffer->setIndexArray(indices);
        return index_buffer;
    }
    template <typename IndexType, typename = std::enable_if_t<std::is_integral_v<IndexType>>>
    void setIndexArray(const std::vector<IndexType>& indices) {
        indices_.clear();

        if (index_type_ == index_types::index_16) {
            size_t array_size = (indices.size() + 1U) / 2U;
            indices_.resize(array_size);
            uint32_t mask = 0xffff;
            const uint8_t shift[] = { 0, 16 };
            for (size_t i = 0; i < indices.size(); ++i) {
                indices_[i / 2] &= ~mask;
                indices_[i / 2] |= (static_cast<uint16_t>(indices[i] & 0xffff) << shift[i % 2]);
                mask = ~mask;
            }
        } else {
            size_t array_size = indices.size();
            indices_.resize(array_size);
            for (size_t i = 0; i < indices.size(); ++i) {
                indices_[i] = static_cast<uint32_t>(indices[i]);
            }
        }
        
        has_cpu_data_ = true;
        has_gpu_data_ = false;
    }
    virtual ~IndexBuffer() override;
    index_types::IndexType index_type() const { return index_type_; }
    virtual size_t data_size() const override { return indices_.size() * sizeof(uint32_t); }
    virtual const void* data() const override { return indices_.data(); };
protected:
    index_types::IndexType index_type_;
    std::vector<uint32_t> indices_;
protected:
    friend class Gfx;
    explicit IndexBuffer(index_types::IndexType index_type, bool keep_cpu_data);
    virtual void clearCpuData() override {
        std::vector<uint32_t> empty_indices;
        std::swap(indices_, empty_indices);
        has_cpu_data_ = false;
    }
};

}