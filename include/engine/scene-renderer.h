#pragma once

#include <memory>
#include <string>

#include "common/common.h"
#include "gfx/renderer.h"
#include "engine/mesh-component.h"

namespace wg {

class SceneRendererRenderData : public IRenderData, public std::enable_shared_from_this<SceneRendererRenderData> {
public:
    ~SceneRendererRenderData() override = default;
    void createGfxResources(class Gfx& gfx) override;

public:
    std::weak_ptr<RenderTarget> weak_render_target;
    std::shared_ptr<UniformBuffer<wg::CameraUniform>> camera_uniform_buffer;

protected:
    friend class SceneRenderer;
    SceneRendererRenderData() = default;
};

struct Camera {
    glm::vec3 position{ 1.0f, 0.0f, 0.0f };
    glm::vec3 center{ 0.0f, 0.0f, 0.0f };
    glm::vec3 up{ 0.0f, 0.0f, 1.0f };
};

class SceneRenderer : public BasicRenderer {
public:
    ~SceneRenderer() override = default;
    [[nodiscard]] static std::shared_ptr<SceneRenderer> Create() {
        return std::shared_ptr<SceneRenderer>(new SceneRenderer());
    }
    
    void setRenderTarget(const std::shared_ptr<RenderTarget>& render_target) { weak_render_target_ = render_target; }

    void addComponent(const std::shared_ptr<MeshComponent>& component) {
        components_.push_back(component);
    }
    void clearComponents() {
        components_.clear();
    }

    void setCamera(Camera camera) { camera_ = camera; }
    const Camera& camera() const { return camera_; }
    
    void updateComponentTransform(const std::shared_ptr<MeshComponent>& component) {
        component->render_data()->model_uniform_buffer->setUniformObject({
            .model_mat = component->transform().transform
        });
        for (const auto& draw_command : component->render_data()->draw_commands) {
            markUniformDirty(draw_command, uniform_attributes::model);
        }
    }

    std::shared_ptr<IRenderData> createRenderData() override;

protected:
    Camera camera_;
    std::weak_ptr<RenderTarget> weak_render_target_;
    std::vector<std::shared_ptr<MeshComponent>> components_;
    std::shared_ptr<SceneRendererRenderData> render_data_;

protected:
    friend class Gfx;
    friend class RenderTarget;
    SceneRenderer() = default;
    CameraUniform createUniformObject();
};


}