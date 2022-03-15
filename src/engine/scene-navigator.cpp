#include "engine/scene-navigator.h"

#include "common/logger.h"

namespace {

[[nodiscard]] auto& logger() {
    static auto logger_ = wg::Logger::Get("engine");
    return *logger_;
}

} // unnamed namespace

namespace wg {

std::shared_ptr<SceneNavigator> SceneNavigator::Create(
    const std::shared_ptr <SceneRenderer>& scene_renderer, const std::shared_ptr <Window>& window
) {
    return std::shared_ptr<SceneNavigator>(new SceneNavigator(scene_renderer, window));
}

SceneNavigator::SceneNavigator(const std::shared_ptr <SceneRenderer>& scene_renderer, const std::shared_ptr <Window>& window)
    : weak_scene_renderer_(scene_renderer), weak_window_(window), camera_(scene_renderer->camera()), velocity_() {}

void SceneNavigator::onKey(keys::Key key, input_actions::InputAction action, input_mods::InputMods mods) {
    if (action == input_actions::press) {
        switch (key) {
        case keys::equal:
            max_speed_ *= 2.f;
            break;
        case keys::minus:
            max_speed_ /= 2.f;
            max_speed_ = std::max(max_speed_, 1.f);
            break;
        default:
            break;
        }
    }
}

void SceneNavigator::onMouseButton(mouse_buttons::MouseButton button, input_actions::InputAction action, input_mods::InputMods mods) {

}

void SceneNavigator::onCursorPos(float x, float y) {

}

void SceneNavigator::tick(float time) {
    // update position
    camera_.position += velocity_ * time;
    camera_.center += velocity_ * time;
    
    // update velocity
    auto force_direction = glm::vec3(0.f);
    if (auto window = weak_window_.lock()) {
        if (window->getKeyState(keys::w) == input_actions::press) {
            force_direction += glm::vec3(0.f, 0.f, -1.f);
        }
        if (window->getKeyState(keys::a) == input_actions::press) {
            force_direction += glm::vec3(-1.f, 0.f, 0.f);
        }
        if (window->getKeyState(keys::s) == input_actions::press) {
            force_direction += glm::vec3(0.f, 0.f, 1.f);
        }
        if (window->getKeyState(keys::d) == input_actions::press) {
            force_direction += glm::vec3(1.f, 0.f, 0.f);
        }
    }
    if (force_direction != glm::vec3(0.f)) {
        force_direction = glm::normalize(force_direction);
        
        auto look_at_direction = camera_.center - camera_.position;
        auto look_at_direction_length = glm::length(look_at_direction);
        if (look_at_direction_length > 1e-4f) {
            look_at_direction = glm::normalize(look_at_direction);
        } else {
            look_at_direction = glm::vec3(0.f);
        }
        auto transform_inv = glm::mat3(1.f);
        if (glm::length(glm::cross(look_at_direction, camera_.up)) > 1e-4f) {
            transform_inv = glm::mat3(glm::lookAt(camera_.position, camera_.center, camera_.up));
        }
        
        force_direction = force_direction * transform_inv;
    }
    
    const float damping_time = 0.5f;
    const float accelerate_time = 0.2f;
    const float stop_speed = 0.1f;
    const float damp = 2.f / damping_time;
    const float acceleration = 2.f / accelerate_time;
    
    float current_speed = glm::length(velocity_);
    if (force_direction == glm::vec3(0.f)) {
        float new_speed = current_speed - current_speed * time * damp;
        if (current_speed <= 0.f || new_speed <= stop_speed * max_speed_) {
            velocity_ = glm::vec3(0.f);
        } else {
            velocity_ = velocity_ * (new_speed / current_speed);
        }
    } else {
        velocity_ += acceleration * time * max_speed_ * force_direction;
        float new_speed = glm::length(velocity_);
        if (new_speed > max_speed_) {
            velocity_ = velocity_ * (max_speed_ / new_speed);
        }
    }
    
    setCameraToRenderer();
}

void SceneNavigator::setCameraToRenderer() {
    if (auto scene_renderer = weak_scene_renderer_.lock()) {
        scene_renderer->setCamera(camera_);
    }
}

} // namespace wg
