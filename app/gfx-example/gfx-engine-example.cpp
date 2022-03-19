#include "platform/platform.h"
#include "gfx/gfx.h"
#include "common/config.h"
#include "engine/material.h"
#include "engine/mesh.h"
#include "engine/mesh-component.h"
#include "engine/scene-renderer.h"
#include "engine/texture.h"
#include "engine/scene-navigator.h"

int main(int, char**) {
    auto app = wg::App::Create("wegnine-gfx-engine-example", std::make_tuple(0, 0, 1));
    auto window = app->createWindow(800, 600, "WEngine gfx engine example", wg::Monitor::GetPrimary(), wg::window_mode::windowed);
    window->setPositionToCenter(wg::Monitor::GetPrimary());

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

    auto grid_material = wg::Material::Create(
        "grid material",
        "shader/static/grid.vert.spv", "shader/static/grid.frag.spv"
    );
    render_data.emplace_back(grid_material->createRenderData());

    auto quad_vertices = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f }, .tex_coord = { 0.f, 1.f } },
        { .position = { 0.5f, -0.5f, 0.f }, .color = { 0.f, 1.f, 0.f }, .tex_coord = { 1.f, 1.f } },
        { .position = { 0.5f, 0.5f, 0.f }, .color = { 0.f, 0.f, 1.f }, .tex_coord = { 1.f, 0.f } },
        { .position = { -0.5f, 0.5f, 0.f }, .color = { 1.f, 1.f, 0.f }, .tex_coord = { 0.f, 0.f } },
    };
    auto quad_indices = std::vector<uint32_t>{ 0, 1, 2, 2, 3, 0 };
    auto quad_mesh = wg::Mesh::CreateFromVertices("quad", quad_vertices, quad_indices);
    render_data.emplace_back(quad_mesh->createRenderData());

    auto bunny_mesh = wg::Mesh::CreateFromObjFile("bunny", "resources/model/bunny.obj");
    render_data.emplace_back(bunny_mesh->createRenderData());

    auto quad_component = wg::MeshComponent::Create("quad");
    quad_component->setTransform(wg::Transform());
    quad_component->setMaterial(material);
    quad_component->setMesh(quad_mesh);
    render_data.emplace_back(quad_component->createRenderData());

    auto bunny_component = wg::MeshComponent::Create("triangle");
    bunny_component->setTransform(wg::Transform());
    bunny_component->setMaterial(grid_material);
    bunny_component->setMesh(bunny_mesh);
    render_data.emplace_back(bunny_component->createRenderData());

    auto render_target = gfx->createRenderTarget(window);

    auto renderer = wg::SceneRenderer::Create();
    auto framebuffer_size = window->extent();
    renderer->setCamera(
        {
            .position = glm::vec3(0.0f, -5.0f, 5.0f),
            .center = glm::vec3(0.0f, 0.0f, 0.0f),
            .up = glm::vec3(0.0f, 0.0f, 1.0f),
            .aspect = static_cast<float>(framebuffer_size.x()) / static_cast<float>(framebuffer_size.y())
        }
    );
    renderer->setRenderTarget(render_target);
    renderer->addComponent(bunny_component);
    renderer->addComponent(quad_component);
    render_data.emplace_back(renderer->createRenderData());

    for (auto&& data : render_data) {
        data->createGfxResources(*gfx);
    }
    assert(renderer->valid());

    quad_component->setTransform(
        wg::Transform{
            .transform = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 3.0f, 1.0f)), glm::vec3(0.0f, 0.0f, -0.3f))
        }
    );
    renderer->updateComponentTransform(quad_component);

    auto navigator = wg::SceneNavigator::Create(renderer, window);
    window->setTick(navigator->tick_func());
    window->setOnKey(navigator->on_key_func());
    window->setOnMouseButton(navigator->on_mouse_button_func());
    window->setOnCursorPos(navigator->on_cursor_pos_func());

    auto time = 0.f;
    app->loop(
        [&](float duration) {
            time += duration;
            auto transform = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            bunny_component->setTransform(
                wg::Transform{
                    .transform = transform
                }
            );

            renderer->updateComponentTransform(bunny_component);

            gfx->render(render_target);
        }
    );

    return 0;
}
