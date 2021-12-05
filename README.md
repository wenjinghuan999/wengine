# WEngine

A toy game engine based on Vulkan.

## Build

### Dependencies

- Vulkan 1.2.198.0
- Other open source libraries on github (configured by `FetchContent`)

### Setting up CMake

This project uses CMake (3.14 or above). Most open source libraries are configured automatically using `FetchContent`, except for Vulkan. You need to make CMake `FindVulkan` work by either setting up proper environment variable, or using `-DCMAKE_PREFIX_PATH=/path/to/VulkanSDK/1.2.198.0`. Then you are ready to go.

### Windows/Linux/MacOS (VSCode)

Using VSCode and CMake plugin should make things easier. Create `settings.json` in `.vscode` folder and add following content into the settings.

```json
{
    "cmake.configureArgs": [
        "-DCMAKE_PREFIX_PATH=/path/to/VulkanSDK/1.2.198.0"
    ],
}
```

Remember to replace your VulkanSDK location. Then use command palette to configure your CMake. Type and select `CMake: Configure` in command palette and follow the instruction. Build the project and you should except executables and libs in `binaries/bin` and `binaries/lib` folder.
