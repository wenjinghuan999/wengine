#include "platform/platform.h"
#include "gfx/gfx.h"
#include "gfx/shader.h"
#include "gfx/render-target.h"
#include "gfx/gfx-pipeline.h"

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

    auto render_target = gfx->createRenderTarget(window);

    auto pipeline = wg::GfxPipeline::Create("simple");
    pipeline->setRenderTarget(render_target);
    pipeline->addShader(vert_shader);
    pipeline->addShader(frag_shader);
    gfx->createPipelineResources(pipeline);

    app.loop();

    return 0;
}
