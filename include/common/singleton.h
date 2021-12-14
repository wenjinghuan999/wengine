#pragma once

namespace wg {

template<typename T>
class Singleton {
public:
    static T& Get() {
        static T instance;
        return instance;
    }
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
protected:
    Singleton() = default;
    ~Singleton() = default;
};

}