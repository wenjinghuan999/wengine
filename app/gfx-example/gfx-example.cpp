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
    gfx->createWindowSurface(window);
    gfx->selectBestPhysicalDevice();
    gfx->createLogicalDevice();

    auto vert_shader = wg::Shader::Load("shader/static/simple.vert.spv", wg::shader_stage::vert);
    auto frag_shader = wg::Shader::Load("shader/static/simple.frag.spv", wg::shader_stage::frag);
    gfx->createShaderResources(vert_shader);
    gfx->createShaderResources(frag_shader);

    auto vertices = std::vector<wg::SimpleVertex>{
        { .position = {  0.f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f } },
        { .position = {  0.5f, 0.5f, 0.f }, .color = { 0.f, 1.f, 0.f } },
        { .position = { -0.5f, 0.5f, 0.f }, .color = { 0.f, 0.f, 1.f } },
    };
    auto vertex_buffer = wg::VertexBuffer<wg::SimpleVertex>::CreateFromVertexArray(vertices);
    gfx->createVertexBufferResources(vertex_buffer);

    auto vertex_factory = wg::GfxVertexFactory{
        { .attribute = wg::vertex_attributes::position, .format = wg::gfx_formats::R32G32B32Sfloat, .location = 0 },
        { .attribute = wg::vertex_attributes::color, .format = wg::gfx_formats::R32G32B32Sfloat, .location = 1 },
    };
    vertex_factory.addVertexBuffer(vertex_buffer);

    auto pipeline = wg::GfxPipeline::Create("simple");
    pipeline->addShader(vert_shader);
    pipeline->addShader(frag_shader);
    pipeline->setVertexFactroy(std::move(vertex_factory));

    auto simple_draw_command = wg::SimpleDrawCommand::Create(pipeline);
    auto renderer = wg::Renderer::Create();
    renderer->addDrawCommand(simple_draw_command);

    auto render_target = gfx->createRenderTarget(window);
    render_target->setRenderer(renderer);
    gfx->submitDrawCommands(render_target);

    app.loop([&gfx, &render_target]() {
        gfx->render(render_target);
    });

    return 0;
}
