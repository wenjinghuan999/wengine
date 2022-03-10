#include "platform/platform.h"
#include "gfx/gfx.h"
#include "engine/material.h"
#include "engine/mesh.h"
#include "engine/mesh-component.h"
#include "engine/scene-renderer.h"
#include "engine/texture.h"
#include "common/config.h"

int main(int, char**) {
    wg::App app("wegnine-gfx-example-anisotropy", std::make_tuple(0, 0, 1));
    
    int width = 800, height = 600;
    auto window_on = app.createWindow(width, height, "anisotropy 8.0");
    auto window_off = app.createWindow(width, height, "anisotropy off");
    wg::Window::SubPlotLayout(wg::Monitor::GetPrimary(), { window_on, window_off }, 1, 2);
    
    wg::EngineConfig::Get().set("gfx-max-sampler-anisotropy", 8.f);

    auto gfx = wg::Gfx::Create(app);

    gfx->createWindowSurface(window_on);
    gfx->createWindowSurface(window_off);
    gfx->selectBestPhysicalDevice();
    gfx->createLogicalDevice();

    std::vector<std::shared_ptr<wg::IRenderData>> render_data;

    auto texture = wg::Texture::Load("resources/img/chessboard.png");
    render_data.emplace_back(texture->createRenderData());

    auto material_on = wg::Material::Create(
        "default material",
        "shader/static/simple.vert.spv", "shader/static/simple.frag.spv"
    );
    material_on->addTexture(texture, { .max_anisotropy = 8.f });
    render_data.emplace_back(material_on->createRenderData());
    
    auto material_off = wg::Material::Create(
        "default material",
        "shader/static/simple.vert.spv", "shader/static/simple.frag.spv"
    );
    material_off->addTexture(texture, { .max_anisotropy = 0.f });
    render_data.emplace_back(material_off->createRenderData());

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
        const std::shared_ptr<wg::Material>& material,
        const std::shared_ptr<wg::Window>& window
    ) -> std::weak_ptr<wg::RenderTarget> {
        
        std::vector<std::shared_ptr<wg::IRenderData>> window_render_data;
        
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
        app.registerWindowData(window, window_render_data);
        app.registerWindowData(window, render_target);
        return render_target->weak_from_this();
    };
    
    auto render_target_on = create_render_target(material_on, window_on);
    auto render_target_off = create_render_target(material_off, window_off);
    window_on.reset();
    window_off.reset();

    app.loop(
        [&](float time) {
            if (auto render_target = render_target_on.lock()) {
                gfx->render(render_target);
            }
            if (auto render_target = render_target_off.lock()) {
                gfx->render(render_target);
            }
        }
    );

    return 0;
}
