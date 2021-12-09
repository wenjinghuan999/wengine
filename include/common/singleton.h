#pragma once

namespace wg {

template<typename T>
class Singleton {
public:
    static T& get() {
        static T instance;
        return instance;
    }
protected:
    Singleton() {}
    ~Singleton() {}
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};

}