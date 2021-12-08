#pragma once

#include "common/common.h"

#include <memory>
#include <string>
#include <vector>

namespace wg {

class PhysicalDevice;
class LogicalDevice;

class Gfx : public IMovable {
public:
    Gfx(const class App& app);
    ~Gfx();
    bool valid() const;

    // Physical device
    void updatePhysicalDevices();
    int getNumPhysicalDevices() const { return static_cast<int>(physical_devices_.size()); }
    const PhysicalDevice& getPhysicalDevice(int index) const { return *physical_devices_[index]; }
    void selectPhysicalDevice(int index);
    void selectBestPhysicalDevice(int hint_index = 0);
    const PhysicalDevice& physical_device() const { return *physical_devices_[current_physical_device_index_]; }
    bool physical_device_valid() const;

    // Logical device
    void createLogicalDevice();
protected:
    std::vector<std::unique_ptr<PhysicalDevice>> physical_devices_;
    int current_physical_device_index_{ -1 };
    std::unique_ptr<LogicalDevice> logical_device_;
protected:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class PhysicalDevice : public IMovable {
public:
    PhysicalDevice(const std::string& name);
    ~PhysicalDevice() = default;
    const std::string& name() const { return name_; };
protected:
    std::string name_;
protected:
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class LogicalDevice : public IMovable {
public:
    LogicalDevice();
    ~LogicalDevice() = default;
protected:
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}