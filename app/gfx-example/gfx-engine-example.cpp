#include "platform/platform.h"
#include "gfx/gfx.h"
#include "engine/material.h"
#include "engine/mesh.h"
#include "engine/mesh-component.h"
#include "engine/scene-renderer.h"
#include "engine/texture.h"

int main(int, char**) {
    wg::App app("wegnine-gfx-engine-example", std::make_tuple(0, 0, 1));
    auto window = app.createWindow(800, 600, "WEngine gfx engine example");

    auto gfx = wg::Gfx::Create(app);

    gfx->createWindowSurface(window);
    gfx->selectBestPhysicalDevice();
    gfx->createLogicalDevice();

    std::vector<std::shared_ptr<wg::IRenderData>> render_data;

    auto texture = wg::Texture::Load("resources/img/statue.png");
    render_data.emplace_back(texture->createRenderData());
    
    auto material = wg::Material::Create(
        "default material",
        "shader/static/simple.vert.spv", "shader/static/simple.frag.spv"
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

    auto triangle_vertices = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f }, .tex_coord = { 0.f, 0.f } },
        { .position = { 0.5f, -0.5f, 0.f }, .color = { 0.f, 1.f, 0.f }, .tex_coord = { 1.f, 0.f } },
        { .position = { 0.0f, 0.5f, 0.f }, .color = { 0.f, 0.f, 1.f }, .tex_coord = { 0.5f, 1.f } },
    };
    auto triangle_mesh = wg::Mesh::CreateFromVertices("triangle", triangle_vertices);
    render_data.emplace_back(triangle_mesh->createRenderData());

    auto quad_component = wg::MeshComponent::Create("quad");
    quad_component->setTransform(wg::Transform());
    quad_component->setMaterial(material);
    quad_component->setMesh(quad_mesh);
    render_data.emplace_back(quad_component->createRenderData());

    auto triangle_component = wg::MeshComponent::Create("triangle");
    triangle_component->setTransform(wg::Transform());
    triangle_component->setMaterial(material);
    triangle_component->setMesh(triangle_mesh);
    render_data.emplace_back(triangle_component->createRenderData());

    auto render_target = gfx->createRenderTarget(window);

    auto renderer = wg::SceneRenderer::Create();
    renderer->setCamera({ glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) });
    renderer->setRenderTarget(render_target);
    renderer->addComponent(quad_component);
    renderer->addComponent(triangle_component);
    render_data.emplace_back(renderer->createRenderData());

    for (auto&& data : render_data) {
        data->createGfxResources(*gfx);
    }
    assert(renderer->valid());
    
    app.loop(
        [&](float time) {

            auto transform = wg::Transform{
                .transform = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f))
            };

            quad_component->setTransform(transform);
            triangle_component->setTransform(transform);
            renderer->updateComponentTransform(quad_component);
            renderer->updateComponentTransform(triangle_component);

            gfx->render(render_target);
        }
    );

    return 0;
}
