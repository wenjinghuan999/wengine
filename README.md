# WEngine

A toy game engine based on Vulkan.

## Build

### Dependencies

- [Vulkan SDK 1.2.189.0](https://vulkan.lunarg.com/)
- Other open source libraries on GitHub (configured by `FetchContent`)

### Setting up CMake

This project uses CMake (3.21 or above). Most open source libraries are configured automatically using `FetchContent`, except for Vulkan. You need to make CMake `FindVulkan` work by either setting up proper environment variable, or using `-DCMAKE_PREFIX_PATH=/path/to/VulkanSDK/1.2.189.0`. Then you are ready to go.

Here are some examples.

### VSCode (Windows/Linux/MacOS)

Using VSCode and CMake plugin should make things easier. Create `settings.json` in `.vscode` folder and add following content into the settings.

```json
{
    "cmake.configureArgs": [
        "-DCMAKE_PREFIX_PATH=/path/to/VulkanSDK/1.2.189.0"
    ]
}
```

Remember to replace your VulkanSDK location. Then use command palette to configure your CMake. Type and select `CMake: Configure` in command palette and follow the instruction. Build the project and you should except executables and libs in `binaries/bin` and `binaries/lib` folder.

### CLion (Windows/Linux/MacOS)

- Open `CMakeLists.txt` as project, and then go to [Settings - CMake](jetbrains://CLion/settings?name=Build%2C+Execution%2C+Deployment--CMake).
- Choose your toolchain (probably `Visual Studio` on Windows).
- Add `-DCMAKE_PREFIX_PATH=/path/to/VulkanSDK/1.2.189.0` to `CMake Options`.
- Change `Build Directory` to `build` (as configured in `.gitignore`).

### Visual Studio (Windows)

- `File` - `Open` - `CMake...`, and choose `CMakeLists.txt`.
- Right click on `CMakeLists.txt` in `Solution Explorer`, go to `CMake Settings (for wengine)`.
- Choose `Toolset` (maybe `msvc_x64_x64`).
- Probably add your vcpkg.cmake to `CMake toolchain file`.
- Change `Build root` to something like `${projectDir}\build\${name}`.
- Add `-DCMAKE_PREFIX_PATH=/path/to/VulkanSDK/1.2.189.0` to `CMake command arguments`.

### CMake Command Line (Ubuntu/Other Linux)

Install Vulkan SDK (via Ubuntu package manager):
```bash
wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-focal.list http://packages.lunarg.com/vulkan/lunarg-vulkan-focal.list
sudo apt update && sudo apt install vulkan-sdk
```
(I'm using Ubuntu 20.04. Change `lunarg-vulkan-focal.list` according to your ubuntu version, like `lunarg-vulkan-bionic.list` for Ubuntu 18.04).

Or download and install Vulkan package manually.
```bash
mkdir vulkansdk && cd vulkansdk
curl https://sdk.lunarg.com/sdk/download/1.2.189.0/linux/vulkansdk-linux-x86_64-1.2.189.0.tar.gz --output vulkansdk-linux-x86_64-1.2.189.0.tar.gz
tar -xzf vulkansdk-linux-x86_64-1.2.189.0.tar.gz
```

We use a quite new version of CMake (3.21), so you probably need to install it.

Installing CMake by source:
```bash
sudo apt install build-essential libssl-dev
curl https://github.com/Kitware/CMake/releases/download/v3.21.4/cmake-3.21.4.tar.gz --output cmake-3.21.4.tar.gz
tar -xzf cmake-3.21.4.tar.gz
cd cmake-3.21.4/
./bootstrap
make -j8
sudo make install
```
If you met `CMake Error: Could not find CMAKE_ROOT !!!`, maybe just run `hash -r`. Or setup `CMAKE_ROOT` manually.

Or installing CMake using script:
```bash
curl https://github.com/Kitware/CMake/releases/download/v3.21.4/cmake-3.21.4-linux-x86_64.sh --output cmake-3.21.4-linux-x86_64.sh
bash cmake-3.21.4-linux-x86_64.sh
```
But you need to set up proper environment variables.

Now check your CMake version with `cmake --version`.

Installing other dependencies: (follow [GLFW compile guide](https://www.glfw.org/docs/3.3/compile_guide.html))
```bash
sudo apt install xorg-dev
```

Clone repo (remember to change working directory):
```bash
git clone git@github.com:wenjinghuan999/wengine.git
mkdir -p wengine/build
cd wengine/build/
```

Run CMake and build:
```bash
cmake ..
make -j8
```

Binaries should be showing up in `binaries` folder.