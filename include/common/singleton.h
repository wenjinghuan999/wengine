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
};

}