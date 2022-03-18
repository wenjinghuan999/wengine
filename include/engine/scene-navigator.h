#pragma once

#include <memory>
#include <string>

#include "platform/platform.h"
#include "engine/scene-renderer.h"
#include "platform/platform-constants.h"

namespace wg {

class SceneNavigator : std::enable_shared_from_this<SceneNavigator> {
public:
    static std::shared_ptr<SceneNavigator> Create(const std::shared_ptr<SceneRenderer>& scene_renderer, const std::shared_ptr<Window>& window);
    std::function<void(float time)> tick_func() {
        return [this](float time) {
            return tick(time);
        };
    }
    std::function<void(keys::Key key, input_actions::InputAction action, input_mods::InputMods mods)> on_key_func() {
        return [this](keys::Key key, input_actions::InputAction action, input_mods::InputMods mods) {
            return onKey(key, action, mods);
        };
    };
    std::function<void(mouse_buttons::MouseButton button, input_actions::InputAction action, input_mods::InputMods mods)> on_mouse_button_func() {
        return [this](mouse_buttons::MouseButton button, input_actions::InputAction action, input_mods::InputMods mods) {
            return onMouseButton(button, action, mods);
        };
    }
    std::function<void(float x, float y)> on_cursor_pos_func() {
        return [this](float x, float y) {
            return onCursorPos(x, y);
        };
    }

protected:
    std::weak_ptr<SceneRenderer> weak_scene_renderer_;
    std::weak_ptr<Window> weak_window_;
    Camera camera_{};
    glm::vec3 velocity_{ 0.f, 0.f, 0.f };
    float max_speed_{ 4.f };
    std::pair<float, float> last_cursor_pos_{ 0.f, 0.f };
    bool last_cursor_pos_valid_{ false };
    input_mods::InputMods last_mods_{ input_mods::none };

protected:
    explicit SceneNavigator(const std::shared_ptr<SceneRenderer>& scene_renderer, const std::shared_ptr<Window>& window);
    void onKey(keys::Key key, input_actions::InputAction action, input_mods::InputMods mods);
    void onMouseButton(mouse_buttons::MouseButton button, input_actions::InputAction action, input_mods::InputMods mods);
    void onCursorPos(float x, float y);
    void tick(float time);
    void setCameraToRenderer();
};

} // namespace wg