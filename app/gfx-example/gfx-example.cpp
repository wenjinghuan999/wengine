#include "platform/platform.h"
#include "gfx/gfx.h"
#include "gfx/shader.h"
#include "gfx/gfx-pipeline.h"
#include "gfx/draw-command.h"
#include "gfx/renderer.h"
#include "gfx/gfx-buffer.h"

int main(int, char**) {
    auto app = wg::App::Create("wegnine-gfx-example", std::make_tuple(0, 0, 1));
    auto window = app->createWindow(800, 600, "WEngine gfx example");
    window->setPositionToCenter(wg::Monitor::GetPrimary());

    auto gfx = wg::Gfx::Create(app);

    gfx->createWindowSurface(window);
    gfx->selectBestPhysicalDevice();
    gfx->createLogicalDevice();

    auto simple_vert_shader = wg::Shader::Load("shader/static/simple.vert.spv", wg::shader_stages::vert);
    auto simple_frag_shader = wg::Shader::Load("shader/static/simple.frag.spv", wg::shader_stages::frag);
    gfx->createShaderResources(simple_vert_shader);
    gfx->createShaderResources(simple_frag_shader);

    auto framebuffer_size = window->extent();
    auto camera_uniform_buffer = wg::UniformBuffer<wg::CameraUniform>::Create();
    auto camera_uniform_object = wg::CameraUniform{
        .view_mat = glm::lookAt(glm::vec3(0.0f, -2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .project_mat = glm::perspective(
            glm::radians(45.0f), static_cast<float>(framebuffer_size.x()) / static_cast<float>(framebuffer_size.y()), 0.1f, 10.0f
        )
    };
    camera_uniform_object.project_mat[1][1] *= -1;
    camera_uniform_buffer->setUniformObject(camera_uniform_object);
    auto quad_model_uniform_buffer = wg::UniformBuffer<wg::ModelUniform>::Create();
    auto triangle_model_uniform_buffer = wg::UniformBuffer<wg::ModelUniform>::Create();

    auto uniform_layout = wg::GfxUniformLayout{}
        .addDescription({ .attribute = wg::uniform_attributes::camera, .binding = 0, .stages = wg::shader_stages::vert | wg::shader_stages::frag })
        .addDescription({ .attribute = wg::uniform_attributes::model, .binding = 1, .stages = wg::shader_stages::vert | wg::shader_stages::frag });

    auto image = wg::Image::Load("resources/img/statue.png");
    gfx->createImageResources(image);
    auto sampler = wg::Sampler::Create(image);
    gfx->createSamplerResources(sampler);
    
    auto sampler_layout = wg::GfxSamplerLayout{}
        .addDescription({ .binding = 2, .stages = wg::shader_stages::frag });
    
    auto quad_vertices = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f }, .tex_coord = { 0.f, 1.f } },
        { .position = { 0.5f, -0.5f, 0.f }, .color = { 0.f, 1.f, 0.f }, .tex_coord = { 1.f, 1.f } },
        { .position = { 0.5f, 0.5f, 0.f }, .color = { 0.f, 0.f, 1.f }, .tex_coord = { 1.f, 0.f } },
        { .position = { -0.5f, 0.5f, 0.f }, .color = { 1.f, 1.f, 0.f }, .tex_coord = { 0.f, 0.f } },
    };
    auto quad_vertex_buffer = wg::VertexBuffer<wg::SimpleVertex>::CreateFromVertexArray(quad_vertices);
    gfx->createVertexBufferResources(quad_vertex_buffer);

    auto quad_indices = std::vector<uint32_t>{ 0, 1, 2, 2, 3, 0 };
    auto quad_index_buffer = wg::IndexBuffer::CreateFromIndexArray(wg::index_types::index_16, quad_indices);
    gfx->createIndexBufferResources(quad_index_buffer);

    auto vertex_factory = wg::GfxVertexFactory{
        { .attribute = wg::vertex_attributes::position, .format = wg::gfx_formats::R32G32B32Sfloat, .location = 0 },
        { .attribute = wg::vertex_attributes::color, .format = wg::gfx_formats::R32G32B32Sfloat, .location = 1 },
        { .attribute = wg::vertex_attributes::tex_coord, .format = wg::gfx_formats::R32G32Sfloat, .location = 2 },
    };

    auto simple_pipeline = wg::GfxPipeline::Create();
    simple_pipeline->addShader(simple_vert_shader);
    simple_pipeline->addShader(simple_frag_shader);
    simple_pipeline->setVertexFactory(std::move(vertex_factory));
    simple_pipeline->setUniformLayout(uniform_layout);
    simple_pipeline->setSamplerLayout(sampler_layout);
    gfx->createPipelineResources(simple_pipeline);

    auto quad_draw_command = wg::SimpleDrawCommand::Create("quad", simple_pipeline);
    quad_draw_command->addVertexBuffer(quad_vertex_buffer);
    quad_draw_command->setIndexBuffer(quad_index_buffer);
    quad_draw_command->addUniformBuffer(quad_model_uniform_buffer);
    quad_draw_command->addSampler(2, sampler);
    assert(quad_draw_command->valid());
    gfx->finishDrawCommand(quad_draw_command);

    auto triangle_vertices = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f }, .tex_coord = { 0.f, 0.f } },
        { .position = { 0.5f, -0.5f, 0.f }, .color = { 0.f, 1.f, 0.f }, .tex_coord = { 1.f, 0.f } },
        { .position = { 0.0f, 0.5f, 0.f }, .color = { 0.f, 0.f, 1.f }, .tex_coord = { 0.5f, 1.f } },
    };
    auto triangle_vertex_buffer = wg::VertexBuffer<wg::SimpleVertex>::CreateFromVertexArray(triangle_vertices);
    gfx->createVertexBufferResources(triangle_vertex_buffer);

    auto triangle_draw_command = wg::SimpleDrawCommand::Create("triangle", simple_pipeline);
    triangle_draw_command->addVertexBuffer(triangle_vertex_buffer);
    triangle_draw_command->addUniformBuffer(triangle_model_uniform_buffer);
    triangle_draw_command->addSampler(2, sampler);
    assert(triangle_draw_command->valid());
    gfx->finishDrawCommand(triangle_draw_command);

    auto renderer = wg::BasicRenderer::Create();
    renderer->addUniformBuffer(camera_uniform_buffer);
    renderer->addDrawCommand(triangle_draw_command);
    renderer->addDrawCommand(quad_draw_command);
    assert(renderer->valid());

    auto render_target = gfx->createRenderTarget(window);
    render_target->setRenderer(renderer);
    gfx->createRenderTargetResources(render_target);
    gfx->submitDrawCommands(render_target);

    quad_model_uniform_buffer->setUniformObject(
        {
            .model_mat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.2f))
        }
    );
    renderer->markUniformDirty(quad_draw_command, wg::uniform_attributes::model);
    
    app->loop(
        [&](float time) {
            auto transform = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            triangle_model_uniform_buffer->setUniformObject(
                {
                    .model_mat = transform
                }
            );
            renderer->markUniformDirty(triangle_draw_command, wg::uniform_attributes::model);

            gfx->render(render_target);
        }
    );

    return 0;
}
