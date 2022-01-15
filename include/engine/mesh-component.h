#pragma once

#include <memory>
#include <string>

#include "common/common.h"
#include "engine/material.h"
#include "engine/mesh.h"

namespace wg {

class MeshComponentRenderData : public IRenderData, public std::enable_shared_from_this<MeshComponentRenderData> {
public:
    ~MeshComponentRenderData() override = default;
    void createGfxResources(class Gfx& gfx) override;
    
public:
    std::shared_ptr<UniformBuffer<wg::ModelUniform>> model_uniform_buffer;
    std::vector<std::shared_ptr<DrawCommand>> draw_commands;
    
protected:
    friend class MeshComponent;
    MeshComponentRenderData() = default;
};

class MeshComponent : public IGfxObject, public std::enable_shared_from_this<MeshComponent> {
public:
    ~MeshComponent() override = default;
    static std::shared_ptr<MeshComponent> Create(const std::string& name) {
        return std::shared_ptr<MeshComponent>(new MeshComponent(name));
    }
    
    [[nodiscard]] const std::string& name() const { return name_; }
    void setTransform(Transform transform) { transform_ = transform; }
    const Transform& transform() const { return transform_; }
    
    void setMaterial(const std::shared_ptr<Material>& material) { material_ = material; }
    void setMesh(const std::shared_ptr<Mesh>& mesh) { mesh_ = mesh; }
    
    std::shared_ptr<IRenderData> createRenderData() override;
    const std::shared_ptr<MeshComponentRenderData>& render_data() const { return render_data_; }
    
protected:
    std::string name_;
    Transform transform_;
    std::shared_ptr<Mesh> mesh_;
    std::shared_ptr<Material> material_;
    std::shared_ptr<MeshComponentRenderData> render_data_;
    
protected:
    explicit MeshComponent(std::string name) : name_(std::move(name)) {}
    ModelUniform createUniformObject();
};

}