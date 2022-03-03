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

struct LocalPacked {
    static std::vector<uint8_t> vert_shader;
    static std::vector<uint8_t> frag_shader;
    static std::vector<uint8_t> grid_vert_shader;
    static std::vector<uint8_t> grid_frag_shader;
    static std::vector<uint8_t> image;
    static std::vector<uint8_t> model;

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
    LocalPacked::write(LocalPacked::vert_shader, "shader/simple.vert.spv");
    LocalPacked::write(LocalPacked::frag_shader, "shader/simple.frag.spv");
    LocalPacked::write(LocalPacked::grid_vert_shader, "shader/grid.vert.spv");
    LocalPacked::write(LocalPacked::grid_frag_shader, "shader/grid.frag.spv");
    CHECK(std::filesystem::exists("shader/simple.vert.spv"));
    CHECK(std::filesystem::exists("shader/simple.frag.spv"));
    CHECK(std::filesystem::exists("shader/grid.vert.spv"));
    CHECK(std::filesystem::exists("shader/grid.frag.spv"));

    // img
    std::filesystem::create_directories("resources");
    LocalPacked::write(LocalPacked::image, "resources/image.png");
    CHECK(std::filesystem::exists("resources/image.png"));
    LocalPacked::write(LocalPacked::model, "resources/model.obj");
    CHECK(std::filesystem::exists("resources/model.obj"));

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
        "shader/simple.vert.spv", "shader/simple.frag.spv"
    );
    material->addTexture(texture);
    render_data.emplace_back(material->createRenderData());

    auto grid_material = wg::Material::Create(
        "grid material",
        "shader/grid.vert.spv", "shader/grid.frag.spv"
    );
    render_data.emplace_back(grid_material->createRenderData());

    auto quad_vertices = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f }, .tex_coord = { 0.f, 1.f } },
        { .position = { 0.5f, -0.5f, 0.f }, .color = { 0.f, 1.f, 0.f }, .tex_coord = { 1.f, 1.f } },
        { .position = { 0.5f, 0.5f, 0.f }, .color = { 0.f, 0.f, 1.f }, .tex_coord = { 1.f, 0.f } },
        { .position = { 0.5f, 0.5f, 0.f }, .color = { 0.f, 0.f, 1.f }, .tex_coord = { 1.f, 0.f } },
        { .position = { -0.5f, 0.5f, 0.f }, .color = { 1.f, 1.f, 0.f }, .tex_coord = { 0.f, 0.f } },
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f }, .tex_coord = { 0.f, 1.f } },
    };
    auto quad_mesh = wg::Mesh::CreateFromVertices("quad", quad_vertices);
    render_data.emplace_back(quad_mesh->createRenderData());
    CHECK(quad_mesh->render_data()->vertex_buffer.get());
    CHECK(quad_mesh->render_data()->vertex_buffer->has_cpu_data());
    CHECK(!quad_mesh->render_data()->vertex_buffer->has_gpu_data());
    CHECK(!quad_mesh->render_data()->index_buffer.get());

    auto bunny_mesh = wg::Mesh::CreateFromObjFile("bunny", "resources/model.obj");
    render_data.emplace_back(bunny_mesh->createRenderData());
    CHECK(bunny_mesh->render_data()->vertex_buffer.get());
    CHECK(bunny_mesh->render_data()->vertex_buffer->has_cpu_data());
    CHECK(!bunny_mesh->render_data()->vertex_buffer->has_gpu_data());
    CHECK(bunny_mesh->render_data()->index_buffer.get());
    CHECK(bunny_mesh->render_data()->index_buffer->has_cpu_data());
    CHECK(!bunny_mesh->render_data()->index_buffer->has_gpu_data());

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

    auto bunny_component = wg::MeshComponent::Create("triangle");
    bunny_component->setTransform(wg::Transform());
    bunny_component->setMaterial(grid_material);
    bunny_component->setMesh(bunny_mesh);
    render_data.emplace_back(bunny_component->createRenderData());
    CHECK(bunny_component->render_data()->model_uniform_buffer.get());
    CHECK(bunny_component->render_data()->model_uniform_buffer->has_cpu_data());
    CHECK(!bunny_component->render_data()->model_uniform_buffer->has_gpu_data());
    CHECK(!bunny_component->render_data()->draw_commands.empty());
    CHECK(bunny_component->render_data()->draw_commands[0].get());
    CHECK(bunny_component->render_data()->draw_commands[0]->valid());

    auto render_target = gfx->createRenderTarget(window);

    auto renderer = wg::SceneRenderer::Create();
    renderer->setCamera({ glm::vec3(0.0f, -5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) });
    renderer->setRenderTarget(render_target);
    renderer->addComponent(quad_component);
    renderer->addComponent(bunny_component);
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

    quad_component->setTransform(
        wg::Transform{
            .transform = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 3.0f, 1.0f)), glm::vec3(0.0f, 0.0f, -0.3f))
        }
    );
    bunny_component->setTransform(
        wg::Transform{
            .transform = glm::mat4(1.0f)
        }
    );
    renderer->updateComponentTransform(quad_component);
    renderer->updateComponentTransform(bunny_component);

    gfx->render(render_target);
}

// Packed data
std::vector<uint8_t> LocalPacked::vert_shader = {
#include "../resources/simple.vert.inc"
};
std::vector<uint8_t> LocalPacked::frag_shader = {
#include "../resources/simple.frag.inc"
};
std::vector<uint8_t> LocalPacked::grid_vert_shader = {
#include "../resources/grid.vert.inc"
};
std::vector<uint8_t> LocalPacked::grid_frag_shader = {
#include "../resources/grid.frag.inc"
};
std::vector<uint8_t> LocalPacked::image = {
#include "../resources/image.inc"
};
std::vector<uint8_t> LocalPacked::model = {
#include "../resources/model.inc"
};
