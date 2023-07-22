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

Please refer to the usual sources of information for your particular operating system. Usually the commands needed to build this project will look like this:

```bash
cmake -B build_pc -DPLATFORM_DESKTOP=ON -DCMAKE_BUILD_TYPE=Release
make -C build_pc -j$(nproc)
```

* crosscompile using mingw64 under ubuntu/debian

```bash
sudo apt-get install g++-mingw-w64-x86-64-posix
cmake -B build_mingw -DPLATFORM_DESKTOP=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="library/cmake/MinGWCross.cmake"
make -C build_mingw -j$(nproc)
```

Also, please note that the `resources` folder must be available in the working directory, otherwise the program will fail to find the shaders.

## Building the demo for WinRT

```powershell
# generate key for codesigning (optional)
openssl req -nodes -newkey rsa:2048 -keyout cert.key -out cert.crt -x509 -days 365 -subj '//CN=borealis' \
  -extensions 'v3_req' -addext 'extendedKeyUsage=codeSigning'
openssl pkcs12 -export -nodes -out winrt/key.pfx -inkey cert.key -in cert.crt -passout pass:

# add xmake repo
xmake repo -a local https://github.com/zeromake/xrepo.git

xmake f -c -y --winrt=y --window=sdl --driver=d3d11
xmake b -y demo
```

## Building the demo for PSV

- install [VITASDK](https://github.com/vitasdk/vdpm)
- install [glfw-vita](https://github.com/xfangfang/glfw-vita/tree/borealis)
- put `*.suprx` files ([PVR_PSP2](https://github.com/GrapheneCt/PVR_PSP2)) to `psv/module`
- put `sce_sys` files to `psv/sce_sys`

> We only need: `libGLESv2.suprx` `libgpu_es4_ext.suprx` `libIMGEGL.suprx` `libpvrPSP2_WSEGL.suprx`
>
> PSV support is experimental at present, and it is very slow to run.

```bash
cmake -B build_psv -DPLATFORM_PSV=ON
make -C build_psv borealis_demo.vpk -j$(nproc)
```

## Building the demo for Android

```shell
# build libromfs generator
./build_libromfs_generator.sh

cd android-project
export JAVA_HOME=/Applications/Android\ Studio.app/Contents/jbr/Contents/Home
export ANDROID_SDK_ROOT=~/Library/Android/sdk
# Once built, the APK will be located in the app/build/outputs/apk/debug directory by default
./gradlew assembleDebug
# Directly install the APK (requires the device or emulator to be connected via adb)
./gradlew installDebug
```


## Building the demo for iOS

```shell
# build libromfs generator
./build_libromfs_generator.sh
```

### 1. Build for arm64 iphoneOS

```shell
# 1. Generate a Xcode project
# IOS_CODE_SIGN_IDENTITY: code is not signed when IOS_CODE_SIGN_IDENTITY is empty
# IOS_GUI_IDENTIFIER: optional, default is com.borealis.demo
cmake -B build-ios -G Xcode -DPLATFORM_IOS=ON -DPLATFORM=OS64 -DDEPLOYMENT_TARGET=13.0 \
  -DIOS_CODE_SIGN_IDENTITY="Your identity" \
  -DIOS_GUI_IDENTIFIER="custom.app.id.here"

# 2. open project in Xcode
open build-ios/*.xcodeproj

# 3. Set up Team and Bundle Identifiers in Xcode, then connect devices to run.
```

### 2. Build for arm64 iphoneOS Simulator

```shell
cmake -B build-ios -G Xcode -DPLATFORM_IOS=ON -DPLATFORM=SIMULATORARM64 -DDEPLOYMENT_TARGET=13.0

# Build
cmake --build build-ios

# Open simulator
open -a Simulator

# After simulator is booted, install app
xcrun simctl install booted build-ios/Debug-iphonesimulator/borealis_demo.app
```

### Including in your project (TL;DR: see the CMakeLists.txt in this repo)
0. Your project must be built as C++17 (`-std=c++1z`). You also need to remove `-fno-rtti` and `-fno-exceptions` if you have them
1. Use a submodule (or even better, a [subrepo](https://github.com/ingydotnet/git-subrepo)) to clone this repository in your project
2. Copy the `resources` folder to the root of your project

Here is my work with borealis: https://github.com/xfangfang/wiliwili
