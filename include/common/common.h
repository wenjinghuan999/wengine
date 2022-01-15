#pragma once

namespace wg {

class IMovable {
public:
    IMovable(const IMovable&) = delete;
    IMovable& operator=(const IMovable&) = delete;
    IMovable(IMovable&&) = default;
    IMovable& operator=(IMovable&&) = default;
    virtual ~IMovable() = default;
    
protected:
    IMovable() = default;
};

class ICopyable {
public:
    ICopyable(const ICopyable&) = default;
    ICopyable& operator=(const ICopyable&) = default;
    ICopyable(ICopyable&&) = default;
    ICopyable& operator=(ICopyable&&) = default;
    virtual ~ICopyable() = default;
    
protected:
    ICopyable() = default;
};

}