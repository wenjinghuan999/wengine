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

std::shared_ptr<Mesh> Mesh::CreateSphere(
    const std::string& name, int level, glm::vec3 color
) {
    std::vector<SimpleVertex> vertices = {
        { .position = { -1.f, 0.f, 0.f }, .normal = { -1.f, 0.f, 0.f }, .color = color, .tex_coord = { 0.f, 0.5f } },  // equator -180
        { .position = { 0.f, 0.f, 1.f }, .normal = { 0.f, 0.f, 1.f }, .color = color, .tex_coord = { 0.125f, 0.f } },   // North Pole
        { .position = { 0.f, 0.f, -1.f }, .normal = { 0.f, 0.f, -1.f }, .color = color, .tex_coord = { 0.125f, 1.f } }, // South Pole
        { .position = { 0.f, -1.f, 0.f }, .normal = { 0.f, -1.f, 0.f }, .color = color, .tex_coord = { 0.25f, 0.5f } }, // equator 270
        { .position = { 0.f, 0.f, 1.f }, .normal = { 0.f, 0.f, 1.f }, .color = color, .tex_coord = { 0.375f, 0.f } },   // North Pole
        { .position = { 0.f, 0.f, -1.f }, .normal = { 0.f, 0.f, -1.f }, .color = color, .tex_coord = { 0.375f, 1.f } }, // South Pole
        { .position = { 1.f, 0.f, 0.f }, .normal = { 1.f, 0.f, 0.f }, .color = color, .tex_coord = { 0.5f, 0.5f } },     // equator 0
        { .position = { 0.f, 0.f, 1.f }, .normal = { 0.f, 0.f, 1.f }, .color = color, .tex_coord = { 0.625f, 0.f } },   // North Pole
        { .position = { 0.f, 0.f, -1.f }, .normal = { 0.f, 0.f, -1.f }, .color = color, .tex_coord = { 0.625f, 1.f } }, // South Pole
        { .position = { 0.f, 1.f, 0.f }, .normal = { 0.f, 1.f, 0.f }, .color = color, .tex_coord = { 0.75f, 0.5f } },   // equator 90
        { .position = { 0.f, 0.f, 1.f }, .normal = { 0.f, 0.f, 1.f }, .color = color, .tex_coord = { 0.875f, 0.f } },   // North Pole
        { .position = { 0.f, 0.f, -1.f }, .normal = { 0.f, 0.f, -1.f }, .color = color, .tex_coord = { 0.875f, 1.f } }, // South Pole
        { .position = { -1.f, 0.f, 0.f }, .normal = { -1.f, 0.f, 0.f }, .color = color, .tex_coord = { 1.f, 0.5f } },  // equator 180
    };
    std::vector<uint32_t> indices = {
        0, 3, 1, 0, 2, 3, 3, 6, 4, 3, 5, 6, 6, 9, 7, 6, 8, 9, 9, 12, 10, 9, 11, 12
    };
    
    auto calcU = [](glm::vec3 p, SimpleVertex v0, SimpleVertex v1) {
        if ((v0.tex_coord.x == 0.f || v0.tex_coord.x == 1.f) && glm::abs(v1.position.z) == 1.f) {
            return v0.tex_coord.x;
        }
        if ((v1.tex_coord.x == 0.f || v1.tex_coord.x == 1.f) && glm::abs(v0.position.z) == 1.f) {
            return v1.tex_coord.x;
        }
        if (v0.tex_coord.x == v1.tex_coord.x && (v0.tex_coord.x == 0.f || v0.tex_coord.x == 1.f)) {
            return v0.tex_coord.x;
        }
        return glm::atan(p.y, p.x) / (2.f * glm::pi<float>()) + 0.5f;
    };
    auto calcV = [](glm::vec3 p) {
        return glm::atan(glm::length(glm::vec2(p.x, p.y)), p.z) / glm::pi<float>();
    };
    
    while (--level > 0) {
        size_t n = indices.size();
        std::vector<uint32_t> new_indices;
        new_indices.reserve(4u * n);
        for (size_t i = 0; i + 3u <= n; i += 3) {
            const auto i0 = static_cast<uint32_t>(indices[i]);
            const auto i1 = static_cast<uint32_t>(indices[i + 1]);
            const auto i2 = static_cast<uint32_t>(indices[i + 2]);
            
            auto v0 = vertices[i0];
            auto v1 = vertices[i1];
            auto v2 = vertices[i2];
            
            auto p3 = glm::normalize(v0.position + v1.position);
            auto p4 = glm::normalize(v1.position + v2.position);
            auto p5 = glm::normalize(v2.position + v0.position);

            auto u3 = glm::vec2{ calcU(p3, v0, v1), calcV(p3) };
            auto u4 = glm::vec2{ calcU(p4, v1, v2), calcV(p4) };
            auto u5 = glm::vec2{ calcU(p5, v2, v0), calcV(p5) };

            const uint32_t i3 = static_cast<uint32_t>(vertices.size()), i4 = i3 + 1u, i5 = i3 + 2u;
            vertices.insert(vertices.end(), {
                { .position = p3, .normal = p3, .color = color, .tex_coord = u3 },
                { .position = p4, .normal = p4, .color = color, .tex_coord = u4 },
                { .position = p5, .normal = p5, .color = color, .tex_coord = u5 }
            });
            
            new_indices.insert(new_indices.end(), {
                i0, i3, i5, i3, i1, i4, i4, i2, i5, i3, i4, i5
            });
        }
        indices = std::move(new_indices);
    }
    
    return CreateFromVertices(name, vertices, indices);
}

std::shared_ptr<Mesh> Mesh::CreateFullScreenTriangle(
    const std::string& name, glm::vec3 color
) {
    std::vector<SimpleVertex> vertices = {
        { .position = { -1.f, -1.f, 1.f - 1e-4f }, .normal = { 0.f, 0.f, -1.f }, .color = color, .tex_coord = { 0.f, 0.f } },
        { .position = { -1.f, 3.f, 1.f - 1e-4f }, .normal = { 0.f, 0.f, -1.f }, .color = color, .tex_coord = { 0.f, 2.f } },
        { .position = { 3.f, -1.f, 1.f - 1e-4f }, .normal = { 0.f, 0.f, -1.f }, .color = color, .tex_coord = { 2.f, 0.f } },
    };

    return CreateFromVertices(name, vertices);
}

std::shared_ptr<Mesh> Mesh::CreateCoordinates(
    const std::string& name
) {
    std::vector<SimpleVertex> vertices = {
        { .position = { 0.f, 0.f, 0.f }, .normal = { 1.f, 0.f, 0.f }, .color = { 1.f, 0.f, 0.f }, .tex_coord = { 0.5f, 0.5f } },
        { .position = { 1.f, 0.f, 0.f }, .normal = { 1.f, 0.f, 0.f }, .color = { 1.f, 0.f, 0.f }, .tex_coord = { 0.f, 0.f } },
        { .position = { 0.f, 0.f, 0.f }, .normal = { 0.f, 1.f, 0.f }, .color = { 0.f, 1.f, 0.f }, .tex_coord = { 0.5f, 0.5f } },
        { .position = { 0.f, 1.f, 0.f }, .normal = { 0.f, 1.f, 0.f }, .color = { 0.f, 1.f, 0.f }, .tex_coord = { 1.f, 0.f } },
        { .position = { 0.f, 0.f, 0.f }, .normal = { 0.f, 0.f, 1.f }, .color = { 0.f, 0.f, 1.f }, .tex_coord = { 0.5f, 0.5f } },
        { .position = { 0.f, 0.f, 1.f }, .normal = { 0.f, 0.f, 1.f }, .color = { 0.f, 0.f, 1.f }, .tex_coord = { 1.f, 1.f } },
    };

    auto mesh = CreateFromVertices(name, vertices);
    mesh->setPrimitiveTopology(primitive_topologies::line_list);
    return mesh;
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