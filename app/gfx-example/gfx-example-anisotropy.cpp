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
    auto app = wg::App::Create("wegnine-gfx-example-anisotropy", std::make_tuple(0, 0, 1));
    
    int width = 400, height = 300;
    std::vector<std::shared_ptr<wg::Window>> windows;
    
    auto max_anisotropies = std::array{ 8.f, 4.f, 2.f, 0.f };
    for (float max_anisotropy : max_anisotropies) {
        windows.emplace_back(app->createWindow(width, height, fmt::format("max anisotropy {}", max_anisotropy)));
    }
    wg::Window::SubPlotLayout(wg::Monitor::GetPrimary(), windows, 2, 2);
    
    wg::EngineConfig::Get().set("gfx-max-sampler-anisotropy", 8.f);

    auto gfx = wg::Gfx::Create(app);

    for (auto&& window : windows) {
        gfx->createWindowSurface(window);
    }
    gfx->selectBestPhysicalDevice();
    gfx->createLogicalDevice();

    std::vector<std::shared_ptr<wg::IRenderData>> render_data;

    auto texture = wg::Texture::Load("resources/img/chessboard.png");
    render_data.emplace_back(texture->createRenderData());

    auto quad_vertices = std::vector<wg::SimpleVertex>{
        { .position = { -50.0f, -50.0f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { 0.f, 100.f } },
        { .position = { 50.0f, -50.0f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { 100.f, 100.f } },
        { .position = { 50.0f, 50.0f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { 100.f, 0.f } },
        { .position = { -50.0f, 50.0f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { 0.f, 0.f } },
    };
    auto quad_indices = std::vector<uint32_t>{ 0, 1, 2, 2, 3, 0 };
    auto quad_mesh = wg::Mesh::CreateFromVertices("quad", quad_vertices, quad_indices);
    render_data.emplace_back(quad_mesh->createRenderData());

    for (auto&& data : render_data) {
        data->createGfxResources(*gfx);
    }
    
    auto create_render_target = [&](
        float max_anisotropy,
        const std::shared_ptr<wg::Window>& window
    ) -> std::weak_ptr<wg::RenderTarget> {
        
        std::vector<std::shared_ptr<wg::IRenderData>> window_render_data;

        auto material = wg::Material::Create(
            fmt::format("default material anisotropy {}", max_anisotropy),
            "shader/static/simple.vert.spv", "shader/static/simple.frag.spv"
        );
        material->addTexture(texture, { .max_anisotropy = max_anisotropy });
        window_render_data.emplace_back(material->createRenderData());
        
        auto quad_component = wg::MeshComponent::Create("quad");
        quad_component->setTransform(wg::Transform());
        quad_component->setMaterial(material);
        quad_component->setMesh(quad_mesh);
        window_render_data.emplace_back(quad_component->createRenderData());

        auto render_target = gfx->createRenderTarget(window);

        auto renderer = wg::SceneRenderer::Create();
        renderer->setCamera({ glm::vec3(0.0f, -5.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) });
        renderer->setRenderTarget(render_target);
        renderer->addComponent(quad_component);
        window_render_data.emplace_back(renderer->createRenderData());

        for (auto&& data : window_render_data) {
            data->createGfxResources(*gfx);
        }
        app->registerWindowData(window, window_render_data);
        app->registerWindowData(window, render_target);
        return render_target->weak_from_this();
    };
    
    std::vector<std::weak_ptr<wg::RenderTarget>> weak_render_targets;
    for (size_t i = 0; i < windows.size(); ++i) {
        weak_render_targets.emplace_back(create_render_target(max_anisotropies[i], windows[i]));
    }
    windows.clear();

    app->loop(
        [&](float duration) {
            for (auto&& weak_render_target : weak_render_targets) {
                if (auto render_target = weak_render_target.lock()) {
                    gfx->render(render_target);
                }
            }
        }
    );

    return 0;
}
