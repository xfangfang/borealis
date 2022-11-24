![borealis logo](https://github.com/natinusala/borealis/blob/main/resources/img/borealis_96.png?raw=true)
# borealis

Controller and TV oriented UI library for PC and Nintendo Switch (libnx).

⚠️ Warning: the project is a WIP - See the Projects tab to follow the journey towards a stable version!

The code for the old version is available in the `legacy` branch.

⚠️ The wiki only contains the documentation for the old version of the library, it has yet to be updated!

- Mimicks the Nintendo Switch system UI, but can also be used to make anything else painlessly
- Hardware acceleration and vector graphics with automatic scaling for TV usage (powered by nanovg)
- Can be ported to new platforms and graphics APIs by providing a nanovg implementation
- Powerful layout engine using flex box as a base for everything (powered by Yoga Layout)
- Automated navigation paths for out-of-the-box controller navigation
- Out of the box touch support
- Define user interfaces using XML and only write code when it matters
- Use and restyle built-in components or make your own from scratch
- Display large amount of data efficiently using recycling lists
- Integrated internationalization and storage systems
- Integrated toolbox (logger, animations, timers, background tasks...)

## Building the demo for Switch

To build for Switch, a standard development environment must first be set up. In order to do so, [refer to the Getting Started guide](https://devkitpro.org/wiki/Getting_Started).

```bash
cmake -B build_switch -DPLATFORM_SWITCH=ON
make -C build_switch borealis_demo.nro -j$(nproc)
```

## Building the demo for PC

To build for PC, the following components are required:

- cmake/make build system
- A C++ compiler supporting the C++17 standard
- GLFW version 3.3 or higher (as a static library)
- GLM version 0.9.8 or higher

Please refer to the usual sources of information for your particular operating system. Usually the commands needed to build this project will look like this:

```bash
cmake -B build_pc -DPLATFORM_DESKTOP=ON
make -C build_pc -j$(nproc)
```

Also, please note that the `resources` folder must be available in the working directory, otherwise the program will fail to find the shaders.

### Including in your project (TL;DR: see the CMakeLists.txt in this repo)
0. Your project must be built as C++17 (`-std=c++1z`). You also need to remove `-fno-rtti` and `-fno-exceptions` if you have them
1. Use a submodule (or even better, a [subrepo](https://github.com/ingydotnet/git-subrepo)) to clone this repository in your project
2. Copy the `resources` folder to the root of your project

Here is my work with borealis: https://github.com/xfangfang/wiliwili