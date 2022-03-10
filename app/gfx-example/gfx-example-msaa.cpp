#include "platform/platform.h"
#include "gfx/gfx.h"
#include "engine/material.h"
#include "engine/mesh.h"
#include "engine/mesh-component.h"
#include "engine/scene-renderer.h"
#include "engine/texture.h"
#include "common/config.h"

#include <array>
#include <fmt/format.h>

int main(int, char**) {
    wg::App app("wegnine-gfx-example-msaa", std::make_tuple(0, 0, 1));

    int width = 400, height = 300;
    std::vector<std::shared_ptr<wg::Window>> windows;

    auto msaa_samples = std::array{ 64, 4, 2, 1 };
    for (int samples : msaa_samples) {
        windows.push_back(app.createWindow(width, height, fmt::format("msaa {}x", samples)));
    }
    wg::Window::SubPlotLayout(wg::Monitor::GetPrimary(), windows, 2, 2);
    
    auto create_gfx_and_render_target = [&](
        const std::shared_ptr<wg::Window>& window
    ) -> std::pair<std::weak_ptr<wg::Gfx>, std::weak_ptr<wg::RenderTarget>> {

        auto gfx = wg::Gfx::Create(app);
        std::vector<std::shared_ptr<wg::IRenderData>> render_data;

        gfx->createWindowSurface(window);
        gfx->selectBestPhysicalDevice();
        gfx->createLogicalDevice();
        
        window->setTitle(fmt::format("msaa {}x", gfx->setup().msaa_samples));

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
        renderer->setCamera({ glm::vec3(-1.0f, -1.5f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) });
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
    
    std::vector<std::pair<std::weak_ptr<wg::Gfx>, std::weak_ptr<wg::RenderTarget>>> gfx_and_render_targets;
    for (size_t i = 0; i < msaa_samples.size(); ++i) {
        wg::EngineConfig::Get().set("gfx-msaa-samples", msaa_samples[i]);
        gfx_and_render_targets.emplace_back(create_gfx_and_render_target(windows[i]));
    }
    windows.clear();

    app.loop(
        [&](float time) {
            for (auto&& gfx_and_render_target : gfx_and_render_targets) {
                if (auto gfx = gfx_and_render_target.first.lock()) {
                    if (auto render_target = gfx_and_render_target.second.lock()) {
                        gfx->render(render_target);
                    }
                }
            }
        }
    );

    return 0;
}

