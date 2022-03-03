#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <fmt/format.h>

#include "gfx/gfx.h"
#include "gfx-private.h"
#include "common/logger.h"
#include "common/config.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

class PhysicalDevice_Test : public wg::PhysicalDevice {
public:
    struct Impl : public wg::PhysicalDevice::Impl {
        Impl() : wg::PhysicalDevice::Impl() {}
    };
};

} // unnamed namespace

namespace std {

doctest::String toString(const std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES>& queue_index_remap) {
    std::stringstream ss;
    bool first_outer = true;
    for (auto&& queue_index_remap_family : queue_index_remap) {
        ss << fmt::format("{}[", first_outer ? "" : " ");
        first_outer = false;

        bool first = true;
        for (auto&& remap : queue_index_remap_family) {
            ss << fmt::format("{}({},{})", first ? "" : " ", remap.first, remap.second);
            first = false;
        }
        ss << "]";
    }
    return ss.str().c_str();
}

} // namespace std

TEST_CASE("allocate queues" * doctest::timeout(1)) {
    PhysicalDevice_Test::Impl impl;

    SUBCASE("compact") {
        impl.num_queues = { 1 };
        impl.queue_supports = { 0b1111 };

        std::array<int, wg::gfx_queues::NUM_QUEUES> required =
            { 1, 1, 1, 1 };
        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap;
        impl.allocateQueues(required, {}, queue_index_remap);

        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap_test = {
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 0 } },
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 0 } },
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 0 } },
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 0 } },
        };
        CHECK_EQ(queue_index_remap, queue_index_remap_test);
    }

    SUBCASE("rich") {
        impl.num_queues = { 16, 2, 4 };
        impl.queue_supports = { 0b1111, 0b0010, 0b0011 };

        std::array<int, wg::gfx_queues::NUM_QUEUES> required =
            { 1, 1, 1, 1 };
        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap;
        impl.allocateQueues(required, {}, queue_index_remap);

        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap_test = {
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 0 } },
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 1 } },
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 2 } },
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 3 } },
        };
        CHECK_EQ(queue_index_remap, queue_index_remap_test);
    }

    SUBCASE("adequate") {
        impl.num_queues = { 1, 1, 1, 1 };
        impl.queue_supports = { 0b1111, 0b1111, 0b1111, 0b1111 };

        std::array<int, wg::gfx_queues::NUM_QUEUES> required =
            { 1, 1, 1, 1 };
        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap;
        impl.allocateQueues(required, {}, queue_index_remap);

        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap_test = {
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 0 } },
            std::vector<std::pair<uint32_t, uint32_t>>{ { 1, 0 } },
            std::vector<std::pair<uint32_t, uint32_t>>{ { 2, 0 } },
            std::vector<std::pair<uint32_t, uint32_t>>{ { 3, 0 } },
        };
        CHECK_EQ(queue_index_remap, queue_index_remap_test);
    }

    SUBCASE("greedy") {
        impl.num_queues = { 16, 2, 4 };
        impl.queue_supports = { 0b1111, 0b0010, 0b0011 };

        std::array<int, wg::gfx_queues::NUM_QUEUES> required =
            { 16, 16, 4, 4 };
        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap;
        impl.allocateQueues(required, {}, queue_index_remap);

        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap_test = {
            std::vector<std::pair<uint32_t, uint32_t>>{
                { 0, 0 },
                { 0, 1 },
                { 0, 2 },
                { 0, 3 },
                { 0, 4 },
                { 0, 5 },
                { 0, 6 }
            },
            std::vector<std::pair<uint32_t, uint32_t>>{
                { 0, 7 },
                { 0, 8 },
                { 0, 9 },
                { 0, 10 },
                { 0, 11 },
                { 0, 12 },
                { 0, 13 }
            },
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 14 } },
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 15 } },
        };
        CHECK_EQ(queue_index_remap, queue_index_remap_test);
    }
}

struct LocalPacked{
    static std::vector<uint8_t> vert_shader;
    static std::vector<uint8_t> frag_shader;
    static std::vector<uint8_t> image;
    
    static void write(const std::vector<uint8_t>& content, const std::string& filename) {
        std::ofstream out(filename, std::ios::binary);
        out.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
    }

    static bool checkSame(const std::vector<uint8_t>& content, const std::string& filename) {
        std::ifstream in(filename, std::ios::binary);
        std::vector<uint8_t> file_content;
        file_content.resize(content.size());
        in.read(reinterpret_cast<char*>(file_content.data()), static_cast<std::streamsize>(content.size()));
        return 0 == std::memcmp(content.data(), file_content.data(), content.size());
    }
};

TEST_CASE("gfx raw" * doctest::timeout(10)) {
    wg::App app("wegnine-gfx-example", std::make_tuple(0, 0, 1));
    
    // Prepare data
    // config
    std::filesystem::create_directories("config");
    {
        std::ofstream out("config/engine.json");
        out << R"({"gfx-separate-transfer": true, "gfx-max-sampler-anisotropy": 8.0})";
    }
    CHECK(std::filesystem::exists("config/engine.json"));
    
    // shader
    std::filesystem::create_directories("shader");
    LocalPacked::write(LocalPacked::vert_shader, "shader/simple.vert.spv");
    LocalPacked::write(LocalPacked::frag_shader, "shader/simple.frag.spv");
    CHECK(std::filesystem::exists("shader/simple.vert.spv"));
    CHECK(LocalPacked::checkSame(LocalPacked::vert_shader, "shader/simple.vert.spv"));
    CHECK(std::filesystem::exists("shader/simple.frag.spv"));
    CHECK(LocalPacked::checkSame(LocalPacked::frag_shader, "shader/simple.frag.spv"));
    
    // img
    std::filesystem::create_directories("resources");
    LocalPacked::write(LocalPacked::image, "resources/image.png");
    CHECK(std::filesystem::exists("resources/image.png"));
    CHECK(LocalPacked::checkSame(LocalPacked::image, "resources/image.png"));
    
    // Begin test
    auto window = app.createWindow(800, 600, "WEngine gfx example");

    auto gfx = wg::Gfx::Create(app);

    gfx->createWindowSurface(window);
    gfx->selectBestPhysicalDevice();
    gfx->createLogicalDevice();

    auto vert_shader = wg::Shader::Load("shader/simple.vert.spv", wg::shader_stages::vert);
    auto frag_shader = wg::Shader::Load("shader/simple.frag.spv", wg::shader_stages::frag);
    gfx->createShaderResources(vert_shader);
    gfx->createShaderResources(frag_shader);
    
    CHECK(vert_shader->loaded());
    CHECK(vert_shader->valid());
    CHECK(frag_shader->loaded());
    CHECK(frag_shader->valid());

    auto camera_uniform_buffer = wg::UniformBuffer<wg::CameraUniform>::Create();
    auto camera_uniform_object = wg::CameraUniform{
        .view_mat = glm::lookAt(glm::vec3(0.0f, -2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .project_mat = glm::perspective(glm::radians(45.0f), 4.f / 3.f, 0.1f, 10.0f)
    };
    camera_uniform_object.project_mat[1][1] *= -1;
    camera_uniform_buffer->setUniformObject(camera_uniform_object);
    CHECK(camera_uniform_buffer->has_cpu_data());
    
    auto quad_model_uniform_buffer = wg::UniformBuffer<wg::ModelUniform>::Create();
    CHECK(!quad_model_uniform_buffer->has_cpu_data());
    auto triangle_model_uniform_buffer = wg::UniformBuffer<wg::ModelUniform>::Create();
    CHECK(!triangle_model_uniform_buffer->has_cpu_data());

    auto uniform_layout = wg::GfxUniformLayout{}
        .addDescription({ .attribute = wg::uniform_attributes::camera, .binding = 0, .stages = wg::shader_stages::vert | wg::shader_stages::frag })
        .addDescription({ .attribute = wg::uniform_attributes::model, .binding = 1, .stages = wg::shader_stages::vert | wg::shader_stages::frag });

    auto image = wg::Image::Load("resources/image.png");
    gfx->createImageResources(image);
    CHECK(image->has_cpu_data());
    CHECK(image->has_gpu_data());

    auto sampler_layout = wg::GfxSamplerLayout{}
        .addDescription({ .binding = 2, .stages = wg::shader_stages::frag });

    auto vertices = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f }, .tex_coord = { 0.f, 1.f } },
        { .position = { 0.5f, -0.5f, 0.f }, .color = { 0.f, 1.f, 0.f }, .tex_coord = { 1.f, 1.f } },
        { .position = { 0.5f, 0.5f, 0.f }, .color = { 0.f, 0.f, 1.f }, .tex_coord = { 1.f, 0.f } },
        { .position = { -0.5f, 0.5f, 0.f }, .color = { 1.f, 1.f, 0.f }, .tex_coord = { 0.f, 0.f } },
    };
    auto vertex_buffer = wg::VertexBuffer<wg::SimpleVertex>::CreateFromVertexArray(vertices);
    CHECK(vertex_buffer->has_cpu_data());
    gfx->createVertexBufferResources(vertex_buffer);
    CHECK(!vertex_buffer->has_cpu_data());
    CHECK(vertex_buffer->has_gpu_data());

    auto indices = std::vector<uint32_t>{ 0, 1, 2, 2, 3, 0 };
    auto index_buffer = wg::IndexBuffer::CreateFromIndexArray(wg::index_types::index_16, indices);
    CHECK(index_buffer->has_cpu_data());
    gfx->createIndexBufferResources(index_buffer);
    CHECK(!index_buffer->has_cpu_data());
    CHECK(index_buffer->has_gpu_data());

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
    CHECK(!quad_draw_command->valid());
    quad_draw_command->addVertexBuffer(vertex_buffer);
    CHECK(!quad_draw_command->valid());
    quad_draw_command->setIndexBuffer(index_buffer);
    quad_draw_command->addUniformBuffer(quad_model_uniform_buffer);
    CHECK(!quad_draw_command->valid());
    quad_draw_command->addImage(2, image);
    CHECK(quad_draw_command->valid());
    gfx->finishDrawCommand(quad_draw_command);

    auto triangle_vertices = std::vector<wg::SimpleVertex>{
        { .position = { -0.5f, -0.5f, 0.f }, .color = { 1.f, 0.f, 0.f }, .tex_coord = { 0.f, 0.f } },
        { .position = { 0.5f, -0.5f, 0.f }, .color = { 0.f, 1.f, 0.f }, .tex_coord = { 1.f, 0.f } },
        { .position = { 0.0f, 0.5f, 0.f }, .color = { 0.f, 0.f, 1.f }, .tex_coord = { 0.5f, 1.f } },
    };
    auto triangle_vertex_buffer = wg::VertexBuffer<wg::SimpleVertex>::CreateFromVertexArray(triangle_vertices);
    gfx->createVertexBufferResources(triangle_vertex_buffer);

    auto triangle_draw_command = wg::SimpleDrawCommand::Create("triangle", pipeline);
    CHECK(!triangle_draw_command->valid());
    triangle_draw_command->addVertexBuffer(triangle_vertex_buffer);
    CHECK(!triangle_draw_command->valid());
    triangle_draw_command->addUniformBuffer(triangle_model_uniform_buffer);
    triangle_draw_command->addImage(2, image);
    CHECK(triangle_draw_command->valid());
    gfx->finishDrawCommand(triangle_draw_command);

    auto renderer = wg::BasicRenderer::Create();
    CHECK(renderer->valid());
    renderer->addDrawCommand(quad_draw_command);
    CHECK(!renderer->valid());
    renderer->addDrawCommand(triangle_draw_command);
    CHECK(!renderer->valid());
    renderer->addUniformBuffer(camera_uniform_buffer);
    CHECK(renderer->valid());

    auto render_target = gfx->createRenderTarget(window);
    render_target->setRenderer(renderer);
    gfx->createRenderTargetResources(render_target);
    gfx->submitDrawCommands(render_target);

    gfx->render(render_target);
    app.wait();
    
    quad_model_uniform_buffer->setUniformObject(
        {
            .model_mat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.2f))
        }
    );
    renderer->markUniformDirty(quad_draw_command, wg::uniform_attributes::model);
    triangle_model_uniform_buffer->setUniformObject(
        {
            .model_mat = glm::mat4(1.0f)
        }
    );
    renderer->markUniformDirty(triangle_draw_command, wg::uniform_attributes::model);

    gfx->render(render_target);
}

// Packed data
std::vector<uint8_t> LocalPacked::vert_shader = {
#include "../resources/simple.vert.inc" 
};
std::vector<uint8_t> LocalPacked::frag_shader = {
#include "../resources/simple.frag.inc"
};
std::vector<uint8_t> LocalPacked::image = {
#include "../resources/image.inc"
};