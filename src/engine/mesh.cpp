#include "gfx/gfx.h"
#include "engine/mesh.h"
#include "common/logger.h"

namespace {

[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("gfx");
    return *logger_;
}

} // unnamed namespace

namespace wg {

void MeshRenderData::createGfxResources(Gfx& gfx) {
    gfx.createVertexBufferResources(vertex_buffer);
    if (index_buffer) {
        gfx.createIndexBufferResources(index_buffer);
    }
}

std::shared_ptr<Mesh> Mesh::CreateFromVertices(
    const std::string& name, std::vector<SimpleVertex> vertices
) {
    auto mesh = std::shared_ptr<Mesh>(new Mesh(name));
    mesh->setVertices(vertices);
    return mesh;
}

std::shared_ptr<Mesh> Mesh::CreateFromVertices(
    const std::string& name, std::vector<SimpleVertex> vertices, std::vector<uint32_t> indices
) {
    auto mesh = std::shared_ptr<Mesh>(new Mesh(name));
    mesh->setVertices(vertices);
    mesh->setIndices(indices);
    return mesh;
}

Mesh::Mesh(const std::string& name)
    : name_(name) {
}

std::shared_ptr<IRenderData> Mesh::createRenderData() {
    render_data_ = std::shared_ptr<MeshRenderData>(new MeshRenderData());
    render_data_->vertex_buffer = wg::VertexBuffer<wg::SimpleVertex>::CreateFromVertexArray(vertices_);
    if (!indices_.empty()) {
        render_data_->index_buffer = wg::IndexBuffer::CreateFromIndexArray(wg::index_types::index_16, indices_);
    }
    return render_data_;
}

} // namespace wg