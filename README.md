# RNBO Unity Audio Plugin

This is experimental software, use at your own risk, and know that anything in this repository could change in the future.


Here we implement a Native Audio Plugin for Unity as well as a helper object that facilitates working with that plugin.

After following the steps below, learn more about working with [RNBO in Unity](docs/README.md).

Check out a working example over at [RNBO Unity Test Project](https://github.com/Cycling74/rnbo.unity.testproj).

![Drum Kit and Orb](docs/images/drumkit-orb.png)

## Project organization

There are two pieces to this project.

The first piece is our **RNBOTypes** package. This contains C# code that your custom plugin 
export will reference. If you create multiple plugins, they will all reference the same **RNBOTypes** 
package.

The second piece is our [NativeAudioPlugin](https://docs.unity3d.com/Manual/AudioMixerNativeAudioPlugin.html) implementation and helper script.  This contains the
adapter code you need to convert your RNBO C++ export into a plugin that you can load in Unity.

### File Structure and Building

The source code of the Plugin itself is in the `src/` directory. In general use of this repository, you shouldn't need to change anything in that directory.

Some notable directories:

| Location                          | Explanation   |
| --------------------------------- | ------------- |
| src/                              | Source for the RNBO Plugin and helper object |
| RNBOTypes/  | A dependency for the built Plugin — you'll install this into your Unity Project |
| export/                           | You can create this folder and export your code here for an easy default experience |
| build/          | You can create this folder to build your Plugin package |

We use CMake to actually build the plugin and its package. [Export your patcher](https://rnbo.cycling74.com/learn/the-cpp-source-code-target-introduction) to a subdirectory
you make called `export` in this repo's root directory, next to the `src` directory, and if your export is called `rnbo_source.cpp`, you can run the following commands in your terminal to build with CMake. 

Start by opening a terminal in the root of this `/rnbo.unity.audioplugin` directory. Then run:

```sh
mkdir build
cd build
cmake .. -DPLUGIN_NAME="My Custom Plugin"
cmake --build .
```
If you change the name of your export from `rnbo_source.cpp` in RNBO's export sidebar, you can either change the CMake variable `RNBO_CLASS_FILE_NAME` in `CMakeLists.txt` or use the CMake flag `-DRNBO_CLASS_FILE_NAME` when you run `cmake ..`

For example:

```sh
mkdir build
cd build
cmake .. -DRNBO_CLASS_FILE_NAME="my_custom_plugin.cpp" -DPLUGIN_NAME="My Custom Plugin"
cmake --build .
```

You can also export your c++ code into and build from some other location, other than `export/` and `build/`.
You'll need to tell CMake where to find your export dir and likely also your RNBO c++ library code.
This can be useful if you want to build several unique plugins from unique RNBO exports.

For example:

```sh
mkdir build-synth
cd build-synth
cmake .. -DRNBO_EXPORT_DIR=/Users/xnor/Documents/export/simple-freq-param -DRNBO_CPP_DIR=/Users/xnor/Documents/export/simple-freq-param/rnbo -DPLUGIN_NAME="Simple Freq Param" 
cmake --build .
```

After building, you should see a folder in your `/build` directory called **My Custom Plugin**. This should contain
all you need to install it as a package in Unity.


## Resources

* [RNBO](https://rnbo.cycling74.com/)
* [RNBO Licensing FAQ](https://support.cycling74.com/hc/en-us/articles/10730637742483-RNBO-Export-Licensing-FAQ)
* [Unity](https://unity.com/)
  * [NativeAudioPlugins](https://github.com/Unity-Technologies/NativeAudioPlugins)
  * [NativeAudioPlugins Docs](https://docs.unity3d.com/Manual/AudioMixerNativeAudioPlugin.html)
  * [Package Manifest Docs](https://docs.unity3d.com/Manual/upm-manifestPkg.html)
  * [Assembly Definition docs](https://docs.unity3d.com/Manual/ScriptCompilationAssemblyDefinitionFiles.html)
* [cmake](https://cmake.org/)

## TODO

* Support additional platforms. Currently we only support MacOS, 64-bit Linux and 64-bit Windows.
* Async Preset Capture
