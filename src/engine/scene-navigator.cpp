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
            camera_.center = camera_.position + (camera_.center - camera_.position) * 2.f;
            break;
        case keys::minus:
            max_speed_ /= 2.f;
            max_speed_ = std::max(max_speed_, 1.f);
            camera_.center = camera_.position + (camera_.center - camera_.position) / 2.f;
            break;
        default:
            break;
        }
    }
    last_mods_ = mods;
}

void SceneNavigator::onMouseButton(mouse_buttons::MouseButton button, input_actions::InputAction action, input_mods::InputMods mods) {
    if (action == input_actions::press) {
        if (auto window = weak_window_.lock()) {
            last_cursor_pos_ = window->getCursorPos();
            last_cursor_pos_valid_ = true;
        }
    } else {
        if (auto window = weak_window_.lock()) {
            if (window->getMouseButtonState(mouse_buttons::left) == input_actions::press
                || window->getMouseButtonState(mouse_buttons::right) == input_actions::press
                || window->getMouseButtonState(mouse_buttons::middle) == input_actions::press) {
                return;
            }
        }
        last_cursor_pos_ = { 0.f, 0.f };
        last_cursor_pos_valid_ = false;
    }
}

void SceneNavigator::onCursorPos(float x, float y) {
    if (!last_cursor_pos_valid_) {
        return;
    }

    if (auto window = weak_window_.lock()) {
        auto size = window->content_size();
        auto sx = static_cast<float>(size.x());
        auto sy = static_cast<float>(size.y());

        auto look = camera_.center - camera_.position;
        auto forward = glm::normalize(look);
        auto right = glm::normalize(glm::cross(forward, camera_.up));
        auto up = glm::cross(right, forward);

        if (window->getMouseButtonState(mouse_buttons::right) == input_actions::press
            && input_mods::IsModActivated(last_mods_, input_mods::alt)) {
            float dx = (x - last_cursor_pos_.first) * glm::pi<float>() / sx;
            float dy = (y - last_cursor_pos_.second) * glm::pi<float>() / sy;

            auto angle_z = glm::acos(glm::dot(glm::vec3(0, 0, 1), forward));
            dy = std::min(std::max(dy, -angle_z + 1e-3f), glm::pi<float>() - angle_z - 1e-3f);

            auto transform = glm::rotate(glm::rotate(glm::mat4(1.f), dx, glm::vec3(0, 0, 1)), -dy, right);
            camera_.position = camera_.center - glm::mat3(transform) * look;

            setCameraToRenderer();
        } else if (window->getMouseButtonState(mouse_buttons::right) == input_actions::press) {
            float dx = (x - last_cursor_pos_.first) * glm::pi<float>() / sx;
            float dy = (y - last_cursor_pos_.second) * glm::pi<float>() / sy;

            auto angle_z = glm::acos(glm::dot(camera_.up, forward));
            dy = std::min(std::max(dy, -angle_z + 1e-3f), glm::pi<float>() - angle_z - 1e-3f);

            auto transform = glm::rotate(glm::rotate(glm::mat4(1.f), -dx, camera_.up), -dy, right);
            camera_.center = camera_.position + glm::mat3(transform) * look;

            setCameraToRenderer();
        } else if (window->getMouseButtonState(mouse_buttons::middle) == input_actions::press) {
            float x0 = x / sx - 0.5f;
            float y0 = y / sy - 0.5f;
            float x1 = last_cursor_pos_.first / sx - 0.5f;
            float y1 = last_cursor_pos_.second / sy - 0.5f;
            float fy = 2.f * glm::length(look) * glm::tan(glm::radians(camera_.fov_y / 2.f));
            float fx = fy * camera_.aspect;
            
            auto p0 = right * x0 * fx - up * y0 * fy;
            auto p1 = right * x1 * fx - up * y1 * fy;
            auto t = p1 - p0;

            camera_.center += t;
            camera_.position += t;

            setCameraToRenderer();
        }
    }
    
    last_cursor_pos_ = std::make_pair(x, y);
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
    const float accelerate_time = 0.25f;
    const float stop_speed = 0.1f;
    const float damp = 2.f / damping_time;
    const float acceleration = 1.f / accelerate_time;
    
    float current_speed = glm::length(velocity_);
    if (force_direction == glm::vec3(0.f)) {
        float new_speed = current_speed - current_speed * time * damp;
        if (current_speed <= 0.f || new_speed <= stop_speed * max_speed_) {
            velocity_ = glm::vec3(0.f);
        } else {
            velocity_ = velocity_ * (new_speed / current_speed);
        }
    } else {
        auto project = glm::dot(velocity_, force_direction);
        auto perp = velocity_ - project * force_direction;
        if (project < 0.f) {
            project = 0.f;
        }
        velocity_ = project * force_direction + perp * (1.f - time * damp);
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
        camera_.aspect = scene_renderer->camera().aspect;
        scene_renderer->setCamera(camera_);
    }
}

} // namespace wg
