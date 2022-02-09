#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <fmt/format.h>

#include "platform/platform.h"
#include "gfx/gfx.h"
#include "engine/material.h"
#include "engine/mesh.h"
#include "engine/mesh-component.h"
#include "engine/scene-renderer.h"
#include "engine/texture.h"
#include "common/logger.h"
#include "common/config.h"

#include <filesystem>
#include <fstream>

struct LocalPacked{
    static std::vector<uint8_t> vert_shader;
    static std::vector<uint8_t> frag_shader;
    static std::vector<uint8_t> image;
    
    static void write(const std::vector<uint8_t>& content, const std::string& filename) {
        std::ofstream out(filename, std::ios::binary);
        out.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
    }
    
    static bool checkSame(const std::vector<uint8_t>& content, const std::string& filename) {
        std::ifstream in(filename, std::ios::binary);
        std::vector<uint8_t> file_content;
        file_content.resize(content.size());
        in.read(reinterpret_cast<char*>(file_content.data()), static_cast<std::streamsize>(content.size()));
        return 0 == std::memcmp(content.data(), file_content.data(), content.size());
    }
};

TEST_CASE("gfx engine" * doctest::timeout(10)) {
    wg::App app("wegnine-gfx-engine-example", std::make_tuple(0, 0, 1));
    
    // Prepare data
    // config
    std::filesystem::create_directories("config");
    {
        std::ofstream out("config/engine.json");
        out << R"({"gfx-separate-transfer": true, "gfx-max-sampler-anisotropy": 8.0})";
    }
    CHECK(std::filesystem::exists("config/engine.json"));
    
    // shader
    std::filesystem::create_directories("shader");
    LocalPacked::write(LocalPacked::vert_shader, "shader/vert.spv");
    LocalPacked::write(LocalPacked::frag_shader, "shader/frag.spv");
    CHECK(std::filesystem::exists("shader/vert.spv"));
    CHECK(LocalPacked::checkSame(LocalPacked::vert_shader, "shader/vert.spv"));
    CHECK(std::filesystem::exists("shader/frag.spv"));
    CHECK(LocalPacked::checkSame(LocalPacked::frag_shader, "shader/frag.spv"));
    
    // img
    std::filesystem::create_directories("resources");
    LocalPacked::write(LocalPacked::image, "resources/image.png");
    CHECK(std::filesystem::exists("resources/image.png"));
    CHECK(LocalPacked::checkSame(LocalPacked::image, "resources/image.png"));
    
    // Begin test
    auto window = app.createWindow(800, 600, "WEngine gfx engine example");

    auto gfx = wg::Gfx::Create(app);

    gfx->createWindowSurface(window);
    gfx->selectBestPhysicalDevice();
    gfx->createLogicalDevice();

    std::vector<std::shared_ptr<wg::IRenderData>> render_data;

    auto texture = wg::Texture::Load("resources/image.png");
    render_data.emplace_back(texture->createRenderData());
    CHECK(texture->render_data()->image.get());
    CHECK(texture->render_data()->image->has_cpu_data());
    CHECK(!texture->render_data()->image->has_gpu_data());

    auto material = wg::Material::Create(
        "default material",
        "shader/vert.spv", "shader/frag.spv"
    );
    material->addTexture(texture);
    render_data.emplace_back(material->createRenderData());

    auto quad_vertices = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f }, .tex_coord = { 0.f, 0.f } },
        { .position = { 0.5f, -0.5f, 0.f }, .color = { 0.f, 1.f, 0.f }, .tex_coord = { 1.f, 0.f } },
        { .position = { 0.5f, 0.5f, 0.f }, .color = { 0.f, 0.f, 1.f }, .tex_coord = { 1.f, 1.f } },
        { .position = { -0.5f, 0.5f, 0.f }, .color = { 1.f, 1.f, 0.f }, .tex_coord = { 0.f, 1.f } },
    };
    auto quad_indices = std::vector<uint32_t>{ 0, 1, 2, 2, 3, 0 };
    auto quad_mesh = wg::Mesh::CreateFromVertices("quad", quad_vertices, quad_indices);
    render_data.emplace_back(quad_mesh->createRenderData());
    CHECK(quad_mesh->render_data()->vertex_buffer.get());
    CHECK(quad_mesh->render_data()->vertex_buffer->has_cpu_data());
    CHECK(!quad_mesh->render_data()->vertex_buffer->has_gpu_data());
    CHECK(quad_mesh->render_data()->index_buffer.get());
    CHECK(quad_mesh->render_data()->index_buffer->has_cpu_data());
    CHECK(!quad_mesh->render_data()->index_buffer->has_gpu_data());

    auto triangle_vertices = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f }, .tex_coord = { 0.f, 0.f } },
        { .position = { 0.5f, -0.5f, 0.f }, .color = { 0.f, 1.f, 0.f }, .tex_coord = { 1.f, 0.f } },
        { .position = { 0.0f, 0.5f, 0.f }, .color = { 0.f, 0.f, 1.f }, .tex_coord = { 0.5f, 1.f } },
    };
    auto triangle_mesh = wg::Mesh::CreateFromVertices("triangle", triangle_vertices);
    render_data.emplace_back(triangle_mesh->createRenderData());
    CHECK(triangle_mesh->render_data()->vertex_buffer.get());
    CHECK(triangle_mesh->render_data()->vertex_buffer->has_cpu_data());
    CHECK(!triangle_mesh->render_data()->vertex_buffer->has_gpu_data());
    CHECK(!triangle_mesh->render_data()->index_buffer.get());

    auto quad_component = wg::MeshComponent::Create("quad");
    quad_component->setTransform(wg::Transform());
    quad_component->setMaterial(material);
    quad_component->setMesh(quad_mesh);
    render_data.emplace_back(quad_component->createRenderData());
    CHECK(quad_component->render_data()->model_uniform_buffer.get());
    CHECK(quad_component->render_data()->model_uniform_buffer->has_cpu_data());
    CHECK(!quad_component->render_data()->model_uniform_buffer->has_gpu_data());
    CHECK(!quad_component->render_data()->draw_commands.empty());
    CHECK(quad_component->render_data()->draw_commands[0].get());
    CHECK(quad_component->render_data()->draw_commands[0]->valid());

    auto triangle_component = wg::MeshComponent::Create("triangle");
    triangle_component->setTransform(wg::Transform());
    triangle_component->setMaterial(material);
    triangle_component->setMesh(triangle_mesh);
    render_data.emplace_back(triangle_component->createRenderData());
    CHECK(triangle_component->render_data()->model_uniform_buffer.get());
    CHECK(triangle_component->render_data()->model_uniform_buffer->has_cpu_data());
    CHECK(!triangle_component->render_data()->model_uniform_buffer->has_gpu_data());
    CHECK(!triangle_component->render_data()->draw_commands.empty());
    CHECK(triangle_component->render_data()->draw_commands[0].get());
    CHECK(triangle_component->render_data()->draw_commands[0]->valid());

    auto render_target = gfx->createRenderTarget(window);

    auto renderer = wg::SceneRenderer::Create();
    renderer->setCamera({ glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) });
    renderer->setRenderTarget(render_target);
    renderer->addComponent(quad_component);
    renderer->addComponent(triangle_component);
    render_data.emplace_back(renderer->createRenderData());
    CHECK_EQ(renderer->render_data()->weak_render_target.lock(), render_target);
    CHECK(renderer->render_data()->camera_uniform_buffer.get());
    CHECK(renderer->render_data()->camera_uniform_buffer->has_cpu_data());
    CHECK(!renderer->render_data()->camera_uniform_buffer->has_gpu_data());
    CHECK(renderer->valid());

    for (auto&& data : render_data) {
        data->createGfxResources(*gfx);
    }

    gfx->render(render_target);
    app.wait();
    
    auto transform = wg::Transform{
        .transform = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f))
    };

    quad_component->setTransform(transform);
    triangle_component->setTransform(transform);
    renderer->updateComponentTransform(quad_component);
    renderer->updateComponentTransform(triangle_component);

    gfx->render(render_target);
}

// Packed data
std::vector<uint8_t> LocalPacked::vert_shader = {
#include "../resources/vert.inc" 
};
std::vector<uint8_t> LocalPacked::frag_shader = {
#include "../resources/frag.inc"
};
std::vector<uint8_t> LocalPacked::image = {
#include "../resources/image.inc"
};