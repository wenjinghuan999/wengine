#include "engine/mesh.h"

#include "common/logger.h"
#include "gfx/gfx.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <utility>

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
    mesh->setVertices(std::move(vertices));
    return mesh;
}

std::shared_ptr<Mesh> Mesh::CreateFromVertices(
    const std::string& name, std::vector<SimpleVertex> vertices, std::vector<uint32_t> indices
) {
    auto mesh = std::shared_ptr<Mesh>(new Mesh(name));
    mesh->setVertices(std::move(vertices));
    mesh->setIndices(std::move(indices));
    return mesh;
}

std::shared_ptr<Mesh> Mesh::CreateFromObjFile(
    const std::string& name, const std::string& filename
) {
    tinyobj::ObjReaderConfig reader_config;
    reader_config.vertex_color = true;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(filename, reader_config)) {
        logger().error("Error loading {}: {}", filename, reader.Error());
    }
    if (!reader.Warning().empty()) {
        logger().warn("Warning loading {}: {}", filename, reader.Warning());
    }

    std::vector<SimpleVertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<SimpleVertex, uint32_t> vertex_index_map;

    // https://github.com/tinyobjloader/tinyobjloader#example-code-new-object-oriented-api
    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();
    for (const auto& shape : shapes) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            auto fv = size_t(shape.mesh.num_face_vertices[f]);

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                // Check if `normal_index` is zero or positive. negative = no normal data
                tinyobj::real_t nx = 0.f, ny = 0.f, nz = 0.f;
                if (idx.normal_index >= 0) {
                    nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                }

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                tinyobj::real_t tx = 0.f, ty = 0.f;
                if (idx.texcoord_index >= 0) {
                    tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                }

                // Optional: vertex colors
                tinyobj::real_t red = attrib.colors[3 * size_t(idx.vertex_index) + 0];
                tinyobj::real_t green = attrib.colors[3 * size_t(idx.vertex_index) + 1];
                tinyobj::real_t blue = attrib.colors[3 * size_t(idx.vertex_index) + 2];

                // OBJ: forward: -z; up: y
                auto vertex = SimpleVertex{
                    .position = { vx, -vz, vy },
                    .normal = { nx, -nz, ny },
                    .color = { red, green, blue },
                    .tex_coord = { tx, ty }
                };

                auto it = vertex_index_map.find(vertex);
                if (it != vertex_index_map.end()) {
                    indices.push_back(it->second);
                } else {
                    auto index = static_cast<uint32_t>(vertices.size());
                    indices.push_back(index);
                    vertex_index_map[vertex] = index;
                    vertices.push_back(vertex);
                }
            }
            index_offset += fv;

            // per-face material
            auto material = shape.mesh.material_ids[f];
        }
    }

    return CreateFromVertices(name, vertices, indices);
}

Mesh::Mesh(std::string name)
    : name_(std::move(name)) {
}

std::shared_ptr<IRenderData> Mesh::createRenderData() {
    render_data_ = std::shared_ptr<MeshRenderData>(new MeshRenderData());
    render_data_->vertex_buffer = wg::VertexBuffer<wg::SimpleVertex>::CreateFromVertexArray(vertices_);
    if (!indices_.empty()) {
        auto index_type = wg::index_types::index_16;
        auto max_index = *std::max_element(indices_.begin(), indices_.end());
        if (max_index > 65535) {
            index_type = wg::index_types::index_32;
        }

        render_data_->index_buffer = wg::IndexBuffer::CreateFromIndexArray(index_type, indices_);
    }
    return render_data_;
}

} // namespace wg