#pragma once

namespace wg {

template <typename T>
class ISingleton {
public:
    [[nodiscard]] static T& Get() {
        static T instance;
        return instance;
    }

public:
    ISingleton(const ISingleton&) = delete;
    ISingleton& operator=(const ISingleton&) = delete;

protected:
    ISingleton() = default;
    ~ISingleton() = default;
};

} // namespace wg