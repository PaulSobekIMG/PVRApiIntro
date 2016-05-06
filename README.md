#PVRApiIntro 

A set of examples introducing the PowerVR Framework including CMake files to target Windows, Linux, Android, iOS and OSX - both for Vulkan and OpenGL ES with one source file.

The CMake files also provide a useful base for writing new cross-platform cross-api applications.

![](README.png)

##Fetch instructions
```
git clone https://github.com/PaulSobekIMG/PVRApiIntro.git
git submodule init
git submodule update
```

##Build Instructions

Pass ```-DAPI=OGLES|Vulkan``` to CMake to choose between Vulkan and OpenGL ES back ends

###Linux
```
cmake -DAPI=OGLES /path/to/PVRApiIntro/Intro1ClearApi/
cmake --build .
```

###Windows
```
cmake -DAPI=OGLES /path/to/PVRApiIntro/Intro1ClearApi/
cmake --build .
```

###OSX
```
cmake -DAPI=OGLES /path/to/PVRApiIntro/Intro1ClearApi/
cmake --build .
```

###Android
```
cmake -DAPI=OGLES -DANDROID_ABIS=armeabi-v7a -DANDROID_NDK=\path\to\android-ndk-r11c-windows-x86_64\android-ndk-r11c -DCMAKE_BUILD_TYPE=Debug -DANDROID_NATIVE_API_LEVEL="19" -DDEMO_SRC_DIR=C:\Path\To\PVRApiIntro\Intro1ClearAPI\ -DANDROID_SDK=C:\Users\path\to\AppData\Local\Android\sdk \path\to\PVRApiIntro\Dependencies\apk_maker\
```

###iOS
```
cmake -GXcode -DCMAKE_TOOLCHAIN_FILE=/path/to/PVRApiIntro/Dependencies/ios-cmake/toolchain/iOS.cmake  -DAPI=OGLES /path/to/PVRApiIntro/Intro1ClearAPI/
open Intro1ClearAPI.xcodeproj/
```

iOS requires the following changes to the project to build

![](ios.png)

1. Set build target to your device and select the project to build (not ALL BUILD)
2. Go to build settings for the target project (not ALL BUILD)
3. Set code signing identity to iOS developer