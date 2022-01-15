#pragma once

#include "common/common.h"
#include "common/math.h"

#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

namespace wg {

namespace vertex_attributes {

enum VertexAttribute {
    none = 0,
    position = 1,
    color = 2,
};

}

namespace index_types {

enum IndexType {
    index_16,
    index_32
};

}

namespace uniform_attributes {

enum UniformAttribute {
    none = -1,
    FRAMEBUFFER_UNIFORMS_START = 0,
    scene = FRAMEBUFFER_UNIFORMS_START,
    camera,
    FRAMEBUFFER_UNIFORMS_END,
    DRAW_COMMAND_UNIFORMS_START = FRAMEBUFFER_UNIFORMS_END,
    material = DRAW_COMMAND_UNIFORMS_START,
    model,
    DRAW_COMMAND_UNIFORMS_END,
    NUM_UNIFORMS = DRAW_COMMAND_UNIFORMS_END
};

}

struct VertexBufferDescription {
    vertex_attributes::VertexAttribute attribute{ vertex_attributes::none };
    gfx_formats::Format format{ gfx_formats::none };
    uint32_t stride{ 0 };
    uint32_t offset{ 0 };
};

template <typename DescriptionType>
void AddDescriptionImplTemplate(std::vector<DescriptionType>& descriptions, DescriptionType description) {
    auto it = std::lower_bound(
        descriptions.begin(), descriptions.end(), description,
        [](const DescriptionType& element, const DescriptionType& value) {
            return element.attribute < value.attribute;
        }
    );
    if (it == descriptions.end() || it->attribute != description.attribute) {
        descriptions.emplace(it, description);
    } else {
        *it = description;
    }
}

struct SimpleVertex {
    glm::vec3 position;
    glm::vec3 color;

    static std::vector<VertexBufferDescription> Descriptions() {
        return {
            {
                vertex_attributes::position, gfx_formats::R32G32B32Sfloat,
                sizeof(SimpleVertex), static_cast<uint32_t>(offsetof(SimpleVertex, position))
            },
            {
                vertex_attributes::color,    gfx_formats::R32G32B32Sfloat,
                sizeof(SimpleVertex), static_cast<uint32_t>(offsetof(SimpleVertex, color))
            },
        };
    }
};

template <typename T>
class is_vertex {
    template <typename U>
    static constexpr auto test(U*)
    -> typename std::is_same<decltype(U::Descriptions()), std::vector<VertexBufferDescription>>::type;

    template <typename>
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
    [[nodiscard]] bool has_cpu_data() const { return has_cpu_data_; }
    [[nodiscard]] bool has_gpu_data() const { return has_gpu_data_; }
    [[nodiscard]] virtual size_t data_size() const = 0;
    [[nodiscard]] virtual const void* data() const = 0;

protected:
    bool keep_cpu_data_;
    bool has_cpu_data_{ false };
    bool has_gpu_data_{ false };

protected:
    friend class Gfx;
    explicit GfxBufferBase(bool keep_cpu_data);
    virtual void clearCpuData() = 0;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class VertexBufferBase : public GfxBufferBase {
public:
    ~VertexBufferBase() override;
    [[nodiscard]] virtual std::vector<VertexBufferDescription> descriptions() const = 0;
    [[nodiscard]] size_t vertex_count() const { return vertex_count_; };

protected:
    size_t vertex_count_{ 0 };

protected:
    explicit VertexBufferBase(bool keep_cpu_data);
};

template <typename VertexType, typename = std::enable_if_t<is_vertex_v<VertexType>>>
class VertexBuffer : public VertexBufferBase, public std::enable_shared_from_this<VertexBuffer<VertexType>> {
public:
    static std::shared_ptr<VertexBuffer> CreateFromVertexArray(std::vector<VertexType> vertices, bool keep_cpu_data = false) {
        auto vertex_buffer = std::shared_ptr<VertexBuffer<VertexType>>(new VertexBuffer<VertexType>(keep_cpu_data));
        vertex_buffer->setVertexArray(std::move(vertices));
        return vertex_buffer;
    }
    void setVertexArray(std::vector<VertexType> vertices) {
        vertex_count_ = vertices.size();
        vertices_ = std::move(vertices);
        has_cpu_data_ = true;
        has_gpu_data_ = false;
    }
    [[nodiscard]] std::vector<VertexBufferDescription> descriptions() const override {
        return VertexType::Descriptions();
    }
    [[nodiscard]] size_t data_size() const override { return vertices_.size() * sizeof(VertexType); }
    [[nodiscard]] const void* data() const override { return vertices_.data(); }

protected:
    std::vector<VertexType> vertices_;

protected:
    friend class Gfx;
    explicit VertexBuffer(bool keep_cpu_data) : VertexBufferBase(keep_cpu_data) {}
    void clearCpuData() override {
        std::vector<VertexType> empty_vertices;
        std::swap(vertices_, empty_vertices);
        has_cpu_data_ = false;
        if (!has_gpu_data_) {
            vertex_count_ = 0;
        }
    }
};

class IndexBuffer : public GfxBufferBase {
public:
    template <typename IndexType, typename = std::enable_if_t<std::is_integral_v<IndexType>>>
    static std::shared_ptr<IndexBuffer>
    CreateFromIndexArray(index_types::IndexType index_type, const std::vector<IndexType>& indices, bool keep_cpu_data = false) {
        auto index_buffer = std::shared_ptr<IndexBuffer>(new IndexBuffer(index_type, keep_cpu_data));
        index_buffer->setIndexArray(indices);
        return index_buffer;
    }

    template <typename IndexType, typename = std::enable_if_t<std::is_integral_v<IndexType>>>
    void setIndexArray(const std::vector<IndexType>& indices) {
        index_count_ = indices.size();
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

    ~IndexBuffer() override;
    [[nodiscard]] size_t data_size() const override { return indices_.size() * sizeof(uint32_t); }
    [[nodiscard]] const void* data() const override { return indices_.data(); };
    [[nodiscard]] index_types::IndexType index_type() const { return index_type_; }
    [[nodiscard]] size_t index_count() const { return index_count_; }

protected:
    index_types::IndexType index_type_;
    std::vector<uint32_t> indices_;
    size_t index_count_{ 0 };

protected:
    friend class Gfx;
    explicit IndexBuffer(index_types::IndexType index_type, bool keep_cpu_data);
    void clearCpuData() override {
        std::vector<uint32_t> empty_indices;
        std::swap(indices_, empty_indices);
        has_cpu_data_ = false;
        if (!has_gpu_data_) {
            index_count_ = 0;
        }
    }
};

struct UniformObjectDescription {
    uniform_attributes::UniformAttribute attribute{ uniform_attributes::none };
};

class UniformBufferBase : public GfxBufferBase {
public:
    ~UniformBufferBase() override;
    [[nodiscard]] virtual UniformObjectDescription description() const = 0;
    static std::shared_ptr<UniformBufferBase> Create(uniform_attributes::UniformAttribute attribute);

protected:
    explicit UniformBufferBase();
};

struct SceneUniform {

    static UniformObjectDescription Description() {
        return { uniform_attributes::scene };
    }
};

struct CameraUniform {
    glm::mat4 view_mat;
    glm::mat4 project_mat;

    static UniformObjectDescription Description() {
        return { uniform_attributes::camera };
    }
};

struct MaterialUniform {

    static UniformObjectDescription Description() {
        return { uniform_attributes::material };
    }
};

struct ModelUniform {
    glm::mat4 model_mat;

    static UniformObjectDescription Description() {
        return { uniform_attributes::model };
    }
};

template <typename T>
class is_uniform_object {
    template <typename U>
    static constexpr auto test(U*)
    -> typename std::is_same<decltype(U::Description()), UniformObjectDescription>::type;

    template <typename>
    static constexpr std::false_type test(...);

    typedef decltype(test<T>(0)) type;

public:
    static constexpr bool value = type::value;
};

template <typename T>
inline constexpr bool is_uniform_object_v = is_uniform_object<T>::value;

template <typename UniformObjectType, typename = std::enable_if_t<is_uniform_object_v<UniformObjectType>>>
class UniformBuffer : public UniformBufferBase, public std::enable_shared_from_this<UniformBuffer<UniformObjectType>> {
public:
    static std::shared_ptr<UniformBuffer> Create() {
        auto uniform_buffer = std::shared_ptr<UniformBuffer<UniformObjectType>>(new UniformBuffer<UniformObjectType>());
        return uniform_buffer;
    }

    void setUniformObject(UniformObjectType uniform_object) {
        uniform_object_ = uniform_object;
        has_cpu_data_ = true;
        has_gpu_data_ = false;
    }

    [[nodiscard]] UniformObjectDescription description() const override {
        return UniformObjectType::Description();
    }

    [[nodiscard]] size_t data_size() const override { return sizeof(UniformObjectType); }
    [[nodiscard]] const void* data() const override { return &uniform_object_; }

protected:
    UniformObjectType uniform_object_;

protected:
    friend class Gfx;
    explicit UniformBuffer() : UniformBufferBase() {}
    void clearCpuData() override {}
};

}