#include "platform/platform.h"
#include "gfx/gfx.h"
#include "gfx/shader.h"
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

    auto vert_shader = wg::Shader::Load("shader/static/simple.vert.spv", wg::shader_stages::vert);
    auto frag_shader = wg::Shader::Load("shader/static/simple.frag.spv", wg::shader_stages::frag);
    gfx->createShaderResources(vert_shader);
    gfx->createShaderResources(frag_shader);

    auto camera_uniform_buffer = wg::UniformBuffer<wg::CameraUniform>::Create();
    camera_uniform_buffer->setUniformObject(
        {
            .view_mat = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            .project_mat = glm::perspective(glm::radians(45.0f), 4.f / 3.f, 0.1f, 10.0f)
        }
    );
    auto model_uniform_buffer = wg::UniformBuffer<wg::ModelUniform>::Create();
    auto model_uniform_buffer2 = wg::UniformBuffer<wg::ModelUniform>::Create();

    auto uniform_layout = wg::GfxUniformLayout{}
        .addDescription({ .attribute = wg::uniform_attributes::camera, .binding = 0, .stages = wg::shader_stages::vert | wg::shader_stages::frag })
        .addDescription({ .attribute = wg::uniform_attributes::model, .binding = 1, .stages = wg::shader_stages::vert | wg::shader_stages::frag });

    auto image = wg::Image::Load("resources/img/statue.png");
    gfx->createImageResources(image);
    
    auto sampler_layout = wg::GfxSamplerLayout{}
        .addDescription({ .binding = 2, .stages = wg::shader_stages::frag });
    
    auto vertices = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f }, .tex_coord = { 0.f, 0.f } },
        { .position = { 0.5f, -0.5f, 0.f }, .color = { 0.f, 1.f, 0.f }, .tex_coord = { 1.f, 0.f } },
        { .position = { 0.5f, 0.5f, 0.f }, .color = { 0.f, 0.f, 1.f }, .tex_coord = { 1.f, 1.f } },
        { .position = { -0.5f, 0.5f, 0.f }, .color = { 1.f, 1.f, 0.f }, .tex_coord = { 0.f, 1.f } },
    };
    auto vertex_buffer = wg::VertexBuffer<wg::SimpleVertex>::CreateFromVertexArray(vertices);
    gfx->createVertexBufferResources(vertex_buffer);

    auto indices = std::vector<uint32_t>{ 0, 1, 2, 2, 3, 0 };
    auto index_buffer = wg::IndexBuffer::CreateFromIndexArray(wg::index_types::index_16, indices);
    gfx->createIndexBufferResources(index_buffer);

    auto vertex_factory = wg::GfxVertexFactory{
        { .attribute = wg::vertex_attributes::position, .format = wg::gfx_formats::R32G32B32Sfloat, .location = 0 },
        { .attribute = wg::vertex_attributes::color, .format = wg::gfx_formats::R32G32B32Sfloat, .location = 1 },
        { .attribute = wg::vertex_attributes::tex_coord, .format = wg::gfx_formats::R32G32Sfloat, .location = 2 },
    };

    auto pipeline = wg::GfxPipeline::Create();
    pipeline->addShader(vert_shader);
    pipeline->addShader(frag_shader);
    pipeline->setVertexFactory(std::move(vertex_factory));
    pipeline->setUniformLayout(uniform_layout);
    pipeline->setSamplerLayout(sampler_layout);
    gfx->createPipelineResources(pipeline);

    auto quad_draw_command = wg::SimpleDrawCommand::Create("quad", pipeline);
    quad_draw_command->addVertexBuffer(vertex_buffer);
    quad_draw_command->setIndexBuffer(index_buffer);
    quad_draw_command->addUniformBuffer(model_uniform_buffer);
    quad_draw_command->addImage(2, image);
    assert(quad_draw_command->valid());
    gfx->finishDrawCommand(quad_draw_command);

    auto vertices2 = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f }, .tex_coord = { 0.f, 0.f } },
        { .position = { 0.5f, -0.5f, 0.f }, .color = { 0.f, 1.f, 0.f }, .tex_coord = { 1.f, 0.f } },
        { .position = { 0.0f, 0.5f, 0.f }, .color = { 0.f, 0.f, 1.f }, .tex_coord = { 0.5f, 1.f } },
    };
    auto vertex_buffer2 = wg::VertexBuffer<wg::SimpleVertex>::CreateFromVertexArray(vertices2);
    gfx->createVertexBufferResources(vertex_buffer2);

    auto triangle_draw_command = wg::SimpleDrawCommand::Create("triangle", pipeline);
    triangle_draw_command->addVertexBuffer(vertex_buffer2);
    triangle_draw_command->addUniformBuffer(model_uniform_buffer2);
    triangle_draw_command->addImage(2, image);
    assert(triangle_draw_command->valid());
    gfx->finishDrawCommand(triangle_draw_command);

    auto renderer = wg::BasicRenderer::Create();
    renderer->addUniformBuffer(camera_uniform_buffer);
    renderer->addDrawCommand(quad_draw_command);
    renderer->addDrawCommand(triangle_draw_command);
    assert(renderer->valid());

    auto render_target = gfx->createRenderTarget(window);
    render_target->setRenderer(renderer);
    gfx->createRenderTargetResources(render_target);
    gfx->submitDrawCommands(render_target);

    app.loop(
        [&](float time) {
            auto transform = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            model_uniform_buffer->setUniformObject(
                {
                    .model_mat = transform
                }
            );
            model_uniform_buffer2->setUniformObject(
                {
                    .model_mat = glm::translate(transform, glm::vec3(0.0f, 0.0f, -0.2f))
                }
            );
            renderer->markUniformDirty(quad_draw_command, wg::uniform_attributes::model);
            renderer->markUniformDirty(triangle_draw_command, wg::uniform_attributes::model);

            gfx->render(render_target);
        }
    );

    return 0;
}
