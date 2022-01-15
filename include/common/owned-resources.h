#pragma once

#include <functional>
#include <map>
#include <memory>

namespace wg {

template <typename T>
class OwnedResourcesBase;
template <typename T>
class OwnedResourceHandle;
template <typename T>
class OwnedResourceWeakHandle;

class OwnedResourceHandleBase : public std::enable_shared_from_this<OwnedResourceHandleBase> {
    template <typename T>
    friend
    class OwnedResourcesBase;

public:
    ~OwnedResourceHandleBase() { on_destroy_(this->weak_from_this()); }

protected:
    OwnedResourceHandleBase() = default;
    std::function<void(const std::weak_ptr<OwnedResourceHandleBase>&)> on_destroy_;
};

using OwnedResourceHandleUntyped = std::shared_ptr<OwnedResourceHandleBase>;
using OwnedResourceWeakHandleUntyped = std::weak_ptr<OwnedResourceHandleBase>;

template <typename T>
class OwnedResourceHandleBaseTyped : public OwnedResourceHandleBase {
    friend class OwnedResourcesBase<T>;
    friend class OwnedResourceHandle<T>;
    friend class OwnedResourceWeakHandle<T>;

protected:
    std::weak_ptr<class OwnedResourcesBase<T>> resources_;
};

template <typename T>
class OwnedResourceHandle : public std::shared_ptr<OwnedResourceHandleBaseTyped<T>> {
public:
    using base = std::shared_ptr<OwnedResourceHandleBaseTyped<T>>;
    OwnedResourceHandle() : base() {}
    explicit OwnedResourceHandle(OwnedResourceHandleBaseTyped<T>* ptr) : base(ptr) {}
    [[nodiscard]] T* data() {
        if (base::get()) {
            if (auto resources = base::get()->resources_.lock()) {
                auto it = resources->resources_.find(base::get()->weak_from_this());
                if (it != resources->resources_.end()) {
                    return it->second.get();
                }
            }
        }
        return nullptr;
    }
    [[nodiscard]] const T* data() const {
        return const_cast<OwnedResourceHandleBaseTyped<T>*>(this)->get();
    }
};

template <typename T>
class OwnedResourceWeakHandle : public std::weak_ptr<OwnedResourceHandleBaseTyped<T>> {
public:
    using base = std::weak_ptr<OwnedResourceHandleBaseTyped<T>>;
    OwnedResourceWeakHandle() : base() {}
    explicit OwnedResourceWeakHandle(const OwnedResourceHandle<T>& handle) : base(handle) {}
    [[nodiscard]] T* data() {
        if (auto shared = this->lock()) {
            if (auto resources = shared->resources_.lock()) {
                auto it = resources->resources_.find(*this);
                if (it != resources->resources_.end()) {
                    return it->second.get();
                }
            }
        }
        return nullptr;
    }
    [[nodiscard]] const T* data() const {
        return const_cast<OwnedResourceWeakHandle<T>*>(this)->data();
    }
};

template <typename T>
class OwnedResources;

template <typename T>
class OwnedResourcesBase : public std::enable_shared_from_this<OwnedResourcesBase<T>> {
protected:
    friend class OwnedResources<T>;
    friend class OwnedResourceHandle<T>;
    friend class OwnedResourceWeakHandle<T>;
    OwnedResourceHandle<T> store(std::unique_ptr<T>&& resource) {
        auto handle = OwnedResourceHandle<T>(new OwnedResourceHandleBaseTyped<T>());
        resources_[handle] = std::move(resource);
        handle->on_destroy_ = [weak_this = this->weak_from_this()](const std::weak_ptr<OwnedResourceHandleBase>& weak_handle) {
            if (auto shared_this = weak_this.lock()) {
                auto it = shared_this->resources_.find(weak_handle);
                if (it != shared_this->resources_.end()) {
                    shared_this->resources_.erase(it);
                }
            }
        };
        handle->resources_ = this->weak_from_this();
        return handle;
    }
    OwnedResourceHandleUntyped storeUntyped(std::unique_ptr<T>&& resource) {
        auto handle = OwnedResourceHandleUntyped(new OwnedResourceHandleBase());
        resources_[handle] = std::move(resource);
        handle->on_destroy_ = [weak_this = this->weak_from_this()](const std::weak_ptr<OwnedResourceHandleBase>& weak_handle) {
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
protected:
    std::shared_ptr<OwnedResourcesBase<T>> base_;

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
    [[nodiscard]] OwnedResourceHandle<T> store(std::unique_ptr<T>&& resource) { return base_->store(std::move(resource)); }
    [[nodiscard]] OwnedResourceHandleUntyped storeUntyped(std::unique_ptr<T>&& resource) { return base_->storeUntyped(std::move(resource)); }
    [[nodiscard]] T& get(const OwnedResourceHandleUntyped& handle) { return *base_->resources_[handle]; }
    [[nodiscard]] const T& get(const OwnedResourceHandleUntyped& handle) const { return *base_->resources_[handle]; }
    [[nodiscard]] iterator begin() { return iterator(base_->resources_.begin()); }
    [[nodiscard]] iterator end() { return iterator(base_->resources_.end()); }
};

}