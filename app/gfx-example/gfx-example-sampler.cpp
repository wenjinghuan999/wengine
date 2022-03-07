#include "platform/platform.h"
#include "gfx/gfx.h"
#include "engine/material.h"
#include "engine/mesh.h"
#include "engine/mesh-component.h"
#include "engine/scene-renderer.h"
#include "engine/texture.h"
#include "common/config.h"

int main(int, char**) {
    wg::EngineConfig::Get().set("gfx-sampler-filter-cubic", true);
    wg::EngineConfig::Get().set("gfx-sampler-mirror-clamp-to-edge", true);
    
    wg::App app("wegnine-gfx-example-anisotropy", std::make_tuple(0, 0, 1));
    
    auto window_filter = app.createWindow(800, 300, "nearest vs linear filter");
    window_filter->setPositionToSubPlot(wg::Monitor::GetPrimary(), 1, 2, 1);
    
    auto window_address_mode = app.createWindow(800, 600, "repeat, mirrored repeat, clamp to edge, clamp to border, mirror clamp to edge");
    window_address_mode->setPositionToSubPlot(wg::Monitor::GetPrimary(), 1, 2, 2);

    auto gfx = wg::Gfx::Create(app);

    gfx->createWindowSurface(window_filter);
    gfx->createWindowSurface(window_address_mode);
    gfx->selectBestPhysicalDevice();
    gfx->createLogicalDevice();

    std::vector<std::shared_ptr<wg::IRenderData>> render_data;

    auto texture = wg::Texture::Load("resources/img/statue.png");
    render_data.emplace_back(texture->createRenderData());
    auto texture_low = wg::Texture::Load("resources/img/statue_32x.png", wg::gfx_formats::R32G32B32A32Sfloat);
    render_data.emplace_back(texture_low->createRenderData());

    auto material_nearest = wg::Material::Create(
        "nearest",
        "shader/static/simple.vert.spv", "shader/static/simple.frag.spv"
    );
    material_nearest->addTexture(
        texture_low,
        { .mag_filter = wg::image_sampler::nearest, .min_filter = wg::image_sampler::nearest, .address_u = wg::image_sampler::clamp_to_edge, .address_v = wg::image_sampler::clamp_to_edge }
    );
    render_data.emplace_back(material_nearest->createRenderData());
    
    auto material_linear = wg::Material::Create(
        "linear",
        "shader/static/simple.vert.spv", "shader/static/simple.frag.spv"
    );
    material_linear->addTexture(
        texture_low,
        { .mag_filter = wg::image_sampler::linear, .min_filter = wg::image_sampler::linear, .address_u = wg::image_sampler::clamp_to_edge, .address_v = wg::image_sampler::clamp_to_edge }
    );
    render_data.emplace_back(material_linear->createRenderData());

    auto material_cubic = wg::Material::Create(
        "cubic",
        "shader/static/simple.vert.spv", "shader/static/simple.frag.spv"
    );
    material_cubic->addTexture(
        texture_low,
        { .mag_filter = wg::image_sampler::cubic, .min_filter = wg::image_sampler::cubic, .address_u = wg::image_sampler::clamp_to_edge, .address_v = wg::image_sampler::clamp_to_edge }
    );
    render_data.emplace_back(material_cubic->createRenderData());

    auto material_repeat = wg::Material::Create(
        "repeat",
        "shader/static/simple.vert.spv", "shader/static/simple.frag.spv"
    );
    material_repeat->addTexture(
        texture,
        { .mag_filter = wg::image_sampler::nearest, .min_filter = wg::image_sampler::nearest, .address_u = wg::image_sampler::repeat, .address_v = wg::image_sampler::repeat }
    );
    render_data.emplace_back(material_repeat->createRenderData());
    
    auto material_mirrored_repeat = wg::Material::Create(
        "mirrored repeat",
        "shader/static/simple.vert.spv", "shader/static/simple.frag.spv"
    );
    material_mirrored_repeat->addTexture(
        texture,
        { .mag_filter = wg::image_sampler::nearest, .min_filter = wg::image_sampler::nearest, .address_u = wg::image_sampler::mirrored_repeat, .address_v = wg::image_sampler::mirrored_repeat }
    );
    render_data.emplace_back(material_mirrored_repeat->createRenderData());
    
    auto material_clamp_edge = wg::Material::Create(
        "clamp to edge",
        "shader/static/simple.vert.spv", "shader/static/simple.frag.spv"
    );
    material_clamp_edge->addTexture(
        texture,
        { .mag_filter = wg::image_sampler::nearest, .min_filter = wg::image_sampler::nearest, .address_u = wg::image_sampler::clamp_to_edge, .address_v = wg::image_sampler::clamp_to_edge }
    );
    render_data.emplace_back(material_clamp_edge->createRenderData());
    
    auto material_clamp_border = wg::Material::Create(
        "clamp to border",
        "shader/static/simple.vert.spv", "shader/static/simple.frag.spv"
    );
    material_clamp_border->addTexture(
        texture,
        { .mag_filter = wg::image_sampler::nearest, .min_filter = wg::image_sampler::nearest, .address_u = wg::image_sampler::clamp_to_border, .address_v = wg::image_sampler::clamp_to_border }
    );
    render_data.emplace_back(material_clamp_border->createRenderData());
    
    auto material_mirror_clamp_edge = wg::Material::Create(
        "mirror clamp to edge",
        "shader/static/simple.vert.spv", "shader/static/simple.frag.spv"
    );
    material_mirror_clamp_edge->addTexture(
        texture,
        { .mag_filter = wg::image_sampler::nearest, .min_filter = wg::image_sampler::nearest, .address_u = wg::image_sampler::mirror_clamp_to_edge, .address_v = wg::image_sampler::mirror_clamp_to_edge }
    );
    render_data.emplace_back(material_mirror_clamp_edge->createRenderData());
    
    auto quad_filter_vertices = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { 0.f, 1.f } },
        { .position = { 0.5f, -0.5f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { 1.f, 1.f } },
        { .position = { 0.5f, 0.5f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { 1.f, 0.f } },
        { .position = { -0.5f, 0.5f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { 0.f, 0.f } },
    };
    auto quad_indices = std::vector<uint32_t>{ 0, 1, 2, 2, 3, 0 };
    auto quad_filter_mesh = wg::Mesh::CreateFromVertices("quad filter", quad_filter_vertices, quad_indices);
    render_data.emplace_back(quad_filter_mesh->createRenderData());
    
    auto quad_address_mode_vertices = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { -1.f, 2.f } },
        { .position = { 0.5f, -0.5f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { 2.f, 2.f } },
        { .position = { 0.5f, 0.5f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { 2.f, -1.f } },
        { .position = { -0.5f, 0.5f, 0.f }, .color = { 0.f, 0.f, 0.f }, .tex_coord = { -1.f, -1.f } },
    };
    auto quad_address_mode_mesh = wg::Mesh::CreateFromVertices("quad filter", quad_address_mode_vertices, quad_indices);
    render_data.emplace_back(quad_address_mode_mesh->createRenderData());

    auto nearest_component = wg::MeshComponent::Create("nearest");
    nearest_component->setTransform(wg::Transform{ glm::translate(glm::mat4(1.f), glm::vec3{ -1.1f, 0.f, 0.f }) });
    nearest_component->setMaterial(material_nearest);
    nearest_component->setMesh(quad_filter_mesh);
    render_data.emplace_back(nearest_component->createRenderData());
    
    auto linear_component = wg::MeshComponent::Create("linear");
    linear_component->setTransform(wg::Transform{ glm::translate(glm::mat4(1.f), glm::vec3{ 0.f, 0.f, 0.f }) });
    linear_component->setMaterial(material_linear);
    linear_component->setMesh(quad_filter_mesh);
    render_data.emplace_back(linear_component->createRenderData());
    
    auto cubic_component = wg::MeshComponent::Create("cubic");
    cubic_component->setTransform(wg::Transform{ glm::translate(glm::mat4(1.f), glm::vec3{ 1.1f, 0.f, 0.f }) });
    cubic_component->setMaterial(material_cubic);
    cubic_component->setMesh(quad_filter_mesh);
    render_data.emplace_back(cubic_component->createRenderData());

    auto render_target_filter = gfx->createRenderTarget(window_filter);
    auto renderer_filter = wg::SceneRenderer::Create();
    auto framebuffer_size = window_filter->extent();
    renderer_filter->setCamera(
        {
            .position = glm::vec3(0.f, 0.f, 2.f),
            .center = glm::vec3(0.f, 0.f, 0.f),
            .up = glm::vec3(0.f, 1.f, 0.f),
            .aspect = static_cast<float>(framebuffer_size.x()) / static_cast<float>(framebuffer_size.y())
        }
    );
    renderer_filter->setRenderTarget(render_target_filter);
    renderer_filter->addComponent(nearest_component);
    renderer_filter->addComponent(linear_component);
    renderer_filter->addComponent(cubic_component);
    render_data.emplace_back(renderer_filter->createRenderData());

    auto repeat_component = wg::MeshComponent::Create("repeat");
    repeat_component->setTransform(wg::Transform{ glm::translate(glm::mat4(1.f), glm::vec3{ -1.1f, 0.6f, 0.f }) });
    repeat_component->setMaterial(material_repeat);
    repeat_component->setMesh(quad_address_mode_mesh);
    render_data.emplace_back(repeat_component->createRenderData());
    
    auto mirrored_repeat_component = wg::MeshComponent::Create("mirrored repeat");
    mirrored_repeat_component->setTransform(wg::Transform{ glm::translate(glm::mat4(1.f), glm::vec3{ 0.f, 0.6f, 0.f }) });
    mirrored_repeat_component->setMaterial(material_mirrored_repeat);
    mirrored_repeat_component->setMesh(quad_address_mode_mesh);
    render_data.emplace_back(mirrored_repeat_component->createRenderData());
    
    auto clamp_edge_component = wg::MeshComponent::Create("clamp to edge");
    clamp_edge_component->setTransform(wg::Transform{ glm::translate(glm::mat4(1.f), glm::vec3{ 1.1f, 0.6f, 0.f }) });
    clamp_edge_component->setMaterial(material_clamp_edge);
    clamp_edge_component->setMesh(quad_address_mode_mesh);
    render_data.emplace_back(clamp_edge_component->createRenderData());
    
    auto clamp_border_component = wg::MeshComponent::Create("clamp to border");
    clamp_border_component->setTransform(wg::Transform{ glm::translate(glm::mat4(1.f), glm::vec3{ -1.1f, -0.6f, 0.f }) });
    clamp_border_component->setMaterial(material_clamp_border);
    clamp_border_component->setMesh(quad_address_mode_mesh);
    render_data.emplace_back(clamp_border_component->createRenderData());
    
    auto mirror_clamp_edge_component = wg::MeshComponent::Create("mirror clamp to edge");
    mirror_clamp_edge_component->setTransform(wg::Transform{ glm::translate(glm::mat4(1.f), glm::vec3{ 0.f, -0.6f, 0.f }) });
    mirror_clamp_edge_component->setMaterial(material_mirror_clamp_edge);
    mirror_clamp_edge_component->setMesh(quad_address_mode_mesh);
    render_data.emplace_back(mirror_clamp_edge_component->createRenderData());

    auto render_target_address_mode = gfx->createRenderTarget(window_address_mode);
    auto renderer_address_mode = wg::SceneRenderer::Create();
    framebuffer_size = window_address_mode->extent();
    renderer_address_mode->setCamera(
        {
            .position = glm::vec3(0.f, 0.f, 3.f),
            .center = glm::vec3(0.f, 0.f, 0.f),
            .up = glm::vec3(0.f, 1.f, 0.f),
            .aspect = static_cast<float>(framebuffer_size.x()) / static_cast<float>(framebuffer_size.y())
        }
    );
    renderer_address_mode->setRenderTarget(render_target_address_mode);
    renderer_address_mode->addComponent(repeat_component);
    renderer_address_mode->addComponent(mirrored_repeat_component);
    renderer_address_mode->addComponent(clamp_edge_component);
    renderer_address_mode->addComponent(clamp_border_component);
    renderer_address_mode->addComponent(mirror_clamp_edge_component);
    render_data.emplace_back(renderer_address_mode->createRenderData());
    
    for (auto&& data : render_data) {
        data->createGfxResources(*gfx);
    }

    app.loop(
        [&](float time) {
            gfx->render(render_target_filter);
            gfx->render(render_target_address_mode);
        }
    );

    return 0;
}
