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

    auto camera_uniform_buffer = wg::UniformBuffer<wg::CameraUniform>::Create();
    camera_uniform_buffer->setUniformObject({
        .view_mat = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .project_mat = glm::perspective(glm::radians(45.0f), 4.f / 3.f, 0.1f, 10.0f)
    });
    auto model_uniform_buffer = wg::UniformBuffer<wg::ModelUniform>::Create();

    gfx->createUniformBufferResources(model_uniform_buffer);
    auto uniform_layout = wg::GfxUniformLayout{}
        .addDescription({ .attribute = wg::uniform_attributes::camera, .binding = 0, .stages = wg::shader_stages::vert | wg::shader_stages::frag })
        .addDescription({ .attribute = wg::uniform_attributes::model, .binding = 1, .stages = wg::shader_stages::vert | wg::shader_stages::frag });

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

    auto quad_pipeline = wg::GfxPipeline::Create();
    quad_pipeline->addShader(vert_shader);
    quad_pipeline->addShader(frag_shader);
    quad_pipeline->setVertexFactory(std::move(vertex_factory));
    quad_pipeline->setUniformLayout(uniform_layout);
    gfx->createPipelineResources(quad_pipeline);

    auto quad_draw_command = wg::SimpleDrawCommand::Create("quad", quad_pipeline);
    quad_draw_command->addUniformBuffer(model_uniform_buffer);
    assert(quad_draw_command->valid());

    auto vertices2 = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f } },
        { .position = {  0.5f, -0.5f, 0.f }, .color = { 0.f, 1.f, 0.f } },
        { .position = {  0.0f,  0.5f, 0.f }, .color = { 0.f, 0.f, 1.f } },
    };
    auto vertex_buffer2 = wg::VertexBuffer<wg::SimpleVertex>::CreateFromVertexArray(vertices2);
    gfx->createVertexBufferResources(vertex_buffer2);
    auto triangle_vertex_factory = wg::GfxVertexFactory{
        { .attribute = wg::vertex_attributes::position, .format = wg::gfx_formats::R32G32B32Sfloat, .location = 0 },
        { .attribute = wg::vertex_attributes::color, .format = wg::gfx_formats::R32G32B32Sfloat, .location = 1 },
    };
    triangle_vertex_factory.addVertexBuffer(vertex_buffer2);

    auto triangle_pipeline = wg::GfxPipeline::Create();
    triangle_pipeline->addShader(vert_shader);
    triangle_pipeline->addShader(frag_shader);
    triangle_pipeline->setVertexFactory(std::move(triangle_vertex_factory));
    triangle_pipeline->setUniformLayout(uniform_layout);
    gfx->createPipelineResources(triangle_pipeline);

    auto triangle_draw_command = wg::SimpleDrawCommand::Create("triangle", triangle_pipeline);
    triangle_draw_command->addUniformBuffer(model_uniform_buffer);
    assert(triangle_draw_command->valid());

    auto renderer = wg::BasicRenderer::Create();
    renderer->addUniformBuffer(camera_uniform_buffer);
    renderer->addDrawCommand(quad_draw_command);
    renderer->addDrawCommand(triangle_draw_command);
    assert(renderer->valid());

    auto render_target = gfx->createRenderTarget(window);
    render_target->setRenderer(renderer);
    gfx->createRenderTargetResources(render_target);
    gfx->submitDrawCommands(render_target);

    app.loop([&](float time) {
        model_uniform_buffer->setUniformObject({
            .model_mat = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f))
        });
        gfx->commitDrawCommandUniformBuffers(render_target, quad_draw_command, wg::uniform_attributes::model);
        gfx->commitDrawCommandUniformBuffers(render_target, triangle_draw_command, wg::uniform_attributes::model);

        gfx->render(render_target);
    });

    return 0;
}
