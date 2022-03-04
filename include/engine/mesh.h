#pragma once

#include <memory>
#include <string>

#include "common/common.h"
#include "gfx/gfx-constants.h"
#include "gfx/gfx-buffer.h"
#include "gfx/draw-command.h"
#include "engine/material.h"

namespace wg {

class MeshRenderData : public IRenderData, public std::enable_shared_from_this<MeshRenderData> {
public:
    ~MeshRenderData() override = default;
    void createGfxResources(class Gfx& gfx) override;
    
public:
    std::shared_ptr<VertexBuffer<SimpleVertex>> vertex_buffer;
    std::shared_ptr<IndexBuffer> index_buffer;
    
protected:
    friend class Mesh;
    MeshRenderData() = default;
};

class Mesh : public IGfxObject, public std::enable_shared_from_this<Mesh> {
public:
    ~Mesh() override = default;
    
    static std::shared_ptr<Mesh> CreateFromVertices(
        const std::string& name, std::vector<SimpleVertex> vertices
    );
    static std::shared_ptr<Mesh> CreateFromVertices(
        const std::string& name, std::vector<SimpleVertex> vertices, std::vector<uint32_t> indices
    );
    static std::shared_ptr<Mesh> CreateFromObjFile(
        const std::string& name, const std::string& filename
    );
    
    const std::vector<wg::SimpleVertex>& vertices() const { return vertices_; }
    const std::vector<uint32_t>& indices() const { return indices_; }
    void setVertices(std::vector<wg::SimpleVertex> vertices) { vertices_ = std::move(vertices); }
    void setIndices(std::vector<uint32_t> indices) { indices_ = std::move(indices); }
    
    [[nodiscard]] const std::string& name() const { return name_; }
    
    std::shared_ptr<IRenderData> createRenderData() override;
    const std::shared_ptr<MeshRenderData>& render_data() const { return render_data_; }
    
protected:
    std::string name_;
    std::vector<wg::SimpleVertex> vertices_;
    std::vector<uint32_t> indices_;
    std::shared_ptr<MeshRenderData> render_data_;
    
protected:
    explicit Mesh(const std::string& name);
};

}