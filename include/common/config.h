#pragma once

#include "common/singleton.h"

#include <any>
#include <string>
#include <memory>
#include <type_traits>

namespace wg {

class Config : public Singleton<Config> {
protected:
    template <typename T>
    struct ConfigHelper;
public:
    void save();
    void load();
    [[nodiscard]] bool dirty() const { return dirty_; }

    template <typename T>
    static constexpr bool is_float_v = std::is_floating_point_v<std::remove_cvref_t<T>>;

    template <typename T>
    static constexpr bool is_int_v = !std::is_same_v<bool, std::remove_cvref_t<T>> && !is_float_v<std::remove_cvref_t<T>> && 
        std::is_convertible_v<std::remove_cvref_t<T>, int> && std::is_convertible_v<int, std::remove_cvref_t<T>>;

    template <typename T>
    class is_char_array {
        template <size_t N>
        static constexpr auto test(const char(&)[N]) -> std::true_type;

        static constexpr std::false_type test(...);

        T t;
        typedef decltype(test(t)) type;

    public:
        static constexpr bool value = type::value;
    };

    template <typename T>
    static constexpr bool is_char_array_v = is_char_array<std::remove_cvref_t<T>>::value;

    template <typename T>
    static constexpr bool is_string_v =
        !std::is_same_v<bool, std::remove_cvref_t<T>> && !is_int_v<std::remove_cvref_t<T>> && !is_float_v<std::remove_cvref_t<T>> &&
            std::is_convertible_v<std::remove_cvref_t<T>, std::string> && std::is_convertible_v<std::string, std::remove_cvref_t<T>>;

    template <typename T>
    static constexpr bool is_bool_v =
        !is_int_v<std::remove_cvref_t<T>> && !is_float_v<std::remove_cvref_t<T>> && !is_string_v<std::remove_cvref_t<T>> &&
            std::is_convertible_v<std::remove_cvref_t<T>, bool> && std::is_convertible_v<bool, std::remove_cvref_t<T>>;

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<is_string_v<T>, T>
    get(const std::string& key) const {
        return static_cast<T>(ConfigHelper<std::string>().get(impl_.get(), key));
    }

    template <typename T>
    std::enable_if_t<is_string_v<T> || is_char_array_v<T> || std::is_same_v<std::remove_cvref_t<T>, char*> || std::is_same_v<std::remove_cvref_t<T>, const char*>, void>
    set(const std::string& key, T&& value) {
        dirty_ = true;
        return ConfigHelper<std::string>().set(impl_.get(), key, static_cast<std::string>(value));
    }

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<is_bool_v<T>, T>
    get(const std::string& key) const {
        return static_cast<T>(ConfigHelper<bool>().get(impl_.get(), key));
    }

    template <typename T>
    std::enable_if_t<is_bool_v<T>, void>
    set(const std::string& key, T&& value) {
        dirty_ = true;
        return ConfigHelper<bool>().set(impl_.get(), key, static_cast<bool>(value));
    }

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<is_int_v<T>, T>
    get(const std::string& key) const {
        return static_cast<T>(ConfigHelper<int>().get(impl_.get(), key));
    }

    template <typename T>
    std::enable_if_t<is_int_v<T>, void>
    set(const std::string& key, T&& value) {
        dirty_ = true;
        return ConfigHelper<int>().set(impl_.get(), key, static_cast<int>(value));
    }

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<is_float_v<T>, T>
    get(const std::string& key) const {
        return static_cast<T>(ConfigHelper<float>().get(impl_.get(), key));
    }

    template <typename T>
    std::enable_if_t<is_float_v<T>, void>
    set(const std::string& key, T&& value) {
        dirty_ = true;
        return ConfigHelper<float>().set(impl_.get(), key, static_cast<float>(value));
    }

protected:
    std::string filename_{ "config/config.json" };
    bool dirty_{ false };

protected:
    Config();
    ~Config();
    friend class Singleton<Config>;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    template <typename T>
    struct ConfigHelper {
        T get(struct Config::Impl* impl, const std::string& key);
        void set(struct Config::Impl* impl, const std::string& key, const T& value);
    };
};

} // namespace wg
