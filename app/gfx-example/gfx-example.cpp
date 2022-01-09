#include "platform/platform.h"
#include "gfx/gfx.h"
#include "gfx/shader.h"
#include "gfx/render-target.h"
#include "gfx/gfx-pipeline.h"
#include "gfx/draw-command.h"
#include "gfx/renderer.h"
#include "gfx/gfx-buffer.h"

int main(int, char**) {
    wg::App app("wegnine-gfx-example", std::make_tuple(0, 0, 1));
    auto window = app.createWindow(800, 600, "WEngine gfx example");

    auto gfx = wg::Gfx::Create(app);

    // Enable "separate_transfer" to use a separate transfer queue
    auto& gfx_features_manager = wg::GfxFeaturesManager::Get();
    gfx_features_manager.enableFeature(wg::gfx_features::separate_transfer);

    gfx->createWindowSurface(window);
    gfx->selectBestPhysicalDevice();
    gfx->createLogicalDevice();

    auto vert_shader = wg::Shader::Load("shader/static/simple.vert.spv", wg::shader_stages::vert);
    auto frag_shader = wg::Shader::Load("shader/static/simple.frag.spv", wg::shader_stages::frag);
    gfx->createShaderResources(vert_shader);
    gfx->createShaderResources(frag_shader);

    auto vertices = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f } },
        { .position = {  0.5f, -0.5f, 0.f }, .color = { 0.f, 1.f, 0.f } },
        { .position = {  0.5f,  0.5f, 0.f }, .color = { 0.f, 0.f, 1.f } },
        { .position = { -0.5f,  0.5f, 0.f }, .color = { 1.f, 1.f, 0.f } },
    };
    auto vertex_buffer = wg::VertexBuffer<wg::SimpleVertex>::CreateFromVertexArray(vertices);
    gfx->createVertexBufferResources(vertex_buffer);
    
    auto indices = std::vector<uint32_t>{ 0, 1, 2, 2, 3, 0 };
    auto index_buffer = wg::IndexBuffer::CreateFromIndexArray(wg::index_types::index_16, indices);
    gfx->createIndexBufferResources(index_buffer);

    auto vertex_factory = wg::GfxVertexFactory{
        { .attribute = wg::vertex_attributes::position, .format = wg::gfx_formats::R32G32B32Sfloat, .location = 0 },
        { .attribute = wg::vertex_attributes::color, .format = wg::gfx_formats::R32G32B32Sfloat, .location = 1 },
    };
    vertex_factory.addVertexBuffer(vertex_buffer);
    vertex_factory.setIndexBuffer(index_buffer);

    auto pipeline = wg::GfxPipeline::Create("simple");
    pipeline->addShader(vert_shader);
    pipeline->addShader(frag_shader);
    pipeline->setVertexFactory(std::move(vertex_factory));
    gfx->createPipelineResources(pipeline);

    auto simple_draw_command = wg::SimpleDrawCommand::Create(pipeline);

    auto vertices2 = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f } },
        { .position = {  0.5f, -0.5f, 0.f }, .color = { 0.f, 1.f, 0.f } },
        { .position = {  0.0f,  0.5f, 0.f }, .color = { 0.f, 0.f, 1.f } },
    };
    auto vertex_buffer2 = wg::VertexBuffer<wg::SimpleVertex>::CreateFromVertexArray(vertices2);
    gfx->createVertexBufferResources(vertex_buffer2);
    auto vertex_factory2 = wg::GfxVertexFactory{
        { .attribute = wg::vertex_attributes::position, .format = wg::gfx_formats::R32G32B32Sfloat, .location = 0 },
        { .attribute = wg::vertex_attributes::color, .format = wg::gfx_formats::R32G32B32Sfloat, .location = 1 },
    };
    vertex_factory2.addVertexBuffer(vertex_buffer2);

    auto pipeline2 = wg::GfxPipeline::Create("simple");
    pipeline2->addShader(vert_shader);
    pipeline2->addShader(frag_shader);
    pipeline2->setVertexFactory(std::move(vertex_factory2));
    gfx->createPipelineResources(pipeline2);

    auto simple_draw_command2 = wg::SimpleDrawCommand::Create(pipeline2);

    auto renderer = wg::BasicRenderer::Create();
    renderer->addDrawCommand(simple_draw_command);
    renderer->addDrawCommand(simple_draw_command2);

    auto render_target = gfx->createRenderTarget(window);
    render_target->setRenderer(renderer);
    gfx->submitDrawCommands(render_target);

    app.loop([&gfx, &render_target]() {
        gfx->render(render_target);
    });

    return 0;
}
