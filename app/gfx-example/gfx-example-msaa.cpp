#include "platform/platform.h"
#include "gfx/gfx.h"
#include "engine/material.h"
#include "engine/mesh.h"
#include "engine/mesh-component.h"
#include "engine/scene-renderer.h"
#include "engine/texture.h"
#include "common/config.h"

int main(int, char**) {
    wg::EngineConfig::Get().set("gfx-msaa-samples", 4);
    wg::App app("wegnine-gfx-example-msaa", std::make_tuple(0, 0, 1));

    int width = 800, height = 600;
    auto window_on = app.createWindow(width, height, "msaa 4x");
    auto window_off = app.createWindow(width, height, "msaa off");
    wg::Window::SubPlotLayout(wg::Monitor::GetPrimary(), { window_on, window_off }, 1, 2);

    auto create_render_target = [&](
        const std::shared_ptr<wg::Window>& window
    ) -> std::pair<std::weak_ptr<wg::Gfx>, std::weak_ptr<wg::RenderTarget>> {

        auto gfx = wg::Gfx::Create(app);
        std::vector<std::shared_ptr<wg::IRenderData>> render_data;

        gfx->createWindowSurface(window);
        gfx->selectBestPhysicalDevice();
        gfx->createLogicalDevice();

        auto texture = wg::Texture::Load("resources/img/statue.png");
        render_data.emplace_back(texture->createRenderData());

        auto quad_vertices = std::vector<wg::SimpleVertex>{
            { .position = { -1.f, -1.f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { 0.f, 1.f } },
            { .position = { 1.f, -1.f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { 1.f, 1.f } },
            { .position = { 1.f, 1.f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { 1.f, 0.f } },
            { .position = { -1.f, 1.f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { 0.f, 0.f } },
        };
        auto quad_indices = std::vector<uint32_t>{ 0, 1, 2, 2, 3, 0 };
        auto quad_mesh = wg::Mesh::CreateFromVertices("quad", quad_vertices, quad_indices);
        render_data.emplace_back(quad_mesh->createRenderData());

        auto material = wg::Material::Create(
            "default material",
            "shader/static/simple.vert.spv", "shader/static/simple.frag.spv"
        );
        material->addTexture(texture);
        render_data.emplace_back(material->createRenderData());
        
        auto quad_component = wg::MeshComponent::Create("quad");
        quad_component->setTransform(wg::Transform());
        quad_component->setMaterial(material);
        quad_component->setMesh(quad_mesh);
        render_data.emplace_back(quad_component->createRenderData());

        auto render_target = gfx->createRenderTarget(window);

        auto renderer = wg::SceneRenderer::Create();
        renderer->setCamera({ glm::vec3(-2.0f, -3.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) });
        renderer->setRenderTarget(render_target);
        renderer->addComponent(quad_component);
        render_data.emplace_back(renderer->createRenderData());

        for (auto&& data : render_data) {
            data->createGfxResources(*gfx);
        }
        app.registerWindowData(window, gfx);
        app.registerWindowData(window, render_data);
        app.registerWindowData(window, render_target);
        return std::make_pair(gfx->weak_from_this(), render_target->weak_from_this());
    };

    wg::EngineConfig::Get().set("gfx-msaa-samples", 4);
    auto gfx_and_render_target_on = create_render_target(window_on);
    wg::EngineConfig::Get().set("gfx-msaa-samples", 0);
    auto gfx_and_render_target_off = create_render_target(window_off);
    window_on.reset();
    window_off.reset();

    app.loop(
        [&](float time) {
            wg::EngineConfig::Get().set("gfx-msaa-samples", 4);
            if (auto gfx = gfx_and_render_target_on.first.lock()) {
                if (auto render_target = gfx_and_render_target_on.second.lock()) {
                    gfx->render(render_target);
                }
            }
            wg::EngineConfig::Get().set("gfx-msaa-samples", 0);
            if (auto gfx = gfx_and_render_target_off.first.lock()) {
                if (auto render_target = gfx_and_render_target_off.second.lock()) {
                    gfx->render(render_target);
                }
            }
        }
    );

    return 0;
}

