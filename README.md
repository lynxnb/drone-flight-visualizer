# Drone Flight Visualizer

This project is a drone flight visualizer built with Vulkan and C++20. It provides a real-time 3D visualization of drone flight paths from a recording of a flight. It currently supports DJI flight logs but it can easily be expanded for other formats. The goal is to provide a tool that can be used to analyze drone flight paths and to help drone pilots improve their flying skills. The project is currently being developed on Windows 11 using Clang and Visual Studio 2022. It should also work on Linux and macOS but this has not been tested yet.

## Prerequisites

- CMake
- Ninja
- C++20 compiler (Clang 17 or GCC 13)
- Vulkan SDK

## Tested Configurations
- Windows 11 with Clang 17 and Visual Studio 2022 MSVC v143
- Windows 11 with GCC 13 and MinGW-w64 11.0.1 (UCRT)

Building with MSVC (`cl.exe`) has been proved to not work because of some issues with the CMake configuration step that tries to redefine targets twice. It hasn't been investigated further yet.

## Installing a C++ compiler on Windows

### Common steps

Winget is the recommended way to install packages on Windows. It is a package manager that is built into Windows 11 and can be installed on Windows 10. It is similar to `apt` on Linux and `brew` on macOS. It is recommended to install the packages using Winget but it is also possible to install them manually. This guide will only show how to install the packages using Winget.

1. Install CMake and Ninja
    ```ps
    winget install Kitware.CMake
    winget install Ninja-build.Ninja
    ```

### Clang and Visual Studio

Clang is used as the compiler and MSVC is used for linking with the Windows SDK. 

1. Install the Visual Studio Build Tools.
2. From within the Visual Studio Installer, install the `MSVC v143` and `Windows 11 SDK` packages **only**.
3. Install the LLVM toolchain for Windows
    ```ps
    winget install LLVM.LLVM
    ```
4. Add the LLVM `bin` folder to your `PATH` environment variable.
5. Test that Clang is installed correctly by running `clang --version` in a terminal. Remember that you need to open a new terminal after modifying the `PATH` environment variable.

### MinGW and GCC or Clang

1. Get MinGW-w64 from https://winlibs.com/
2. Extract the archive to a folder of your choice, preferably without spaces, and add the `bin` folder to your `PATH` environment variable.
3. Test that GCC is installed correctly by running `gcc --version` in a terminal. Remember that you need to open a new terminal after modifying the `PATH` environment variable.
4. Test that Clang is installed correctly by running `clang --version` in a terminal. Remember that you need to open a new terminal after modifying the `PATH` environment variable.

## Building

The usual CMake commands can be used to build the project. Dependencies are downloaded automatically by CMake using [CPM](https://github.com/cpm-cmake/CPM.cmake).

For example, to build the project in Debug mode using Clang, run the following commands from the root of the project:
```ps
cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Binaries will be placed in the `bin` folder in the root of the project.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
