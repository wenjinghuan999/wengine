#pragma once

#include <functional>
#include <map>
#include <memory>

namespace wg {

class OwnedResourceHandleBase : public std::enable_shared_from_this<OwnedResourceHandleBase> {
    template <typename T>
    friend class OwnedResourcesBase;
public:
    ~OwnedResourceHandleBase() { on_destroy_(weak_from_this()); }
protected:
    OwnedResourceHandleBase() = default;
    std::function<void(const std::weak_ptr<OwnedResourceHandleBase>&)> on_destroy_;
};

using OwnedResourceHandle = std::shared_ptr<OwnedResourceHandleBase>;

template <typename T>
class OwnedResources;

template <typename T>
class OwnedResourcesBase : public std::enable_shared_from_this<OwnedResourcesBase<T>> {
protected:
    friend class OwnedResources<T>;
    OwnedResourceHandle store(std::unique_ptr<T>&& resource) {
        auto handle = OwnedResourceHandle(new OwnedResourceHandleBase());
        resources_[handle] = std::move(resource);
        handle->on_destroy_ = [weak_this=weak_from_this()](const std::weak_ptr<OwnedResourceHandleBase>& weak_handle) {
            if (auto shared_this = weak_this.lock()) {
                auto it = shared_this->resources_.find(weak_handle);
                if (it != shared_this->resources_.end()) {
                    shared_this->resources_.erase(it);
                }
            }
        };
        return handle;
    }
    static std::shared_ptr<OwnedResourcesBase> Create() {
        return std::shared_ptr<OwnedResourcesBase>(new OwnedResourcesBase());
    }
    OwnedResourcesBase() {}
    std::map<std::weak_ptr<OwnedResourceHandleBase>, std::unique_ptr<T>, std::owner_less<std::weak_ptr<OwnedResourceHandleBase>>> resources_;
};

template <typename T>
class OwnedResources {
public:
    class iterator : public decltype(base_->resources_)::iterator {
    public:
        using base = typename decltype(base_->resources_)::iterator;
        explicit iterator(base b) : base(b) {}
        T& operator*() { return *static_cast<base>(*this)->second; }
        const T& operator*() const { return *static_cast<base>(*this)->second; }
    };
public:
    OwnedResources() : base_(OwnedResourcesBase<T>::Create()) {}
    [[nodiscard]] OwnedResourceHandle store(std::unique_ptr<T>&& resource) { return base_->store(std::move(resource)); }
    [[nodiscard]] T& get(const OwnedResourceHandle& handle) { return base_->resources_[handle]; }
    [[nodiscard]] const T& get(const OwnedResourceHandle& handle) const { return base_->resources_[handle]; }
    [[nodiscard]] iterator begin() { return iterator(base_->resources_.begin()); }
    [[nodiscard]] iterator end() { return iterator(base_->resources_.end()); }
protected: 
    std::shared_ptr<OwnedResourcesBase<T>> base_;
};

}