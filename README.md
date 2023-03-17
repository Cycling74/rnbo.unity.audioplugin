# RNBO Unity Audio Plugin

This is experimental software, use at your own risk, and know that anything in this repository could change in the future.


Here we implement a Native Audio Plugin for Unity as well as a helper object that facilitates working with that plugin.

After following the steps below, learn more about working with [RNBO in Unity](docs/RNBO_IN_UNITY.md).

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
| export/                           | You'll need to create this folder, and export your code here |
| build/          | You'll need to create this folder, your built Plugin package will end up here |
| src/                              | Source for the RNBO Plugin and helper object |
| RNBOTypes/  | A dependency for the built Plugin — you'll install this into your Unity Project |

We use Cmake to actually build the plugin and its package. [Export your patcher](https://rnbo.cycling74.com/learn/the-cpp-source-code-target-introduction) to a subdirectory
you make called `export` in this repo's root directory, next to the `src` directory, and if your export is called `rnbo_source.cpp`, you can run the following commands in your terminal to build with Cmake. 

Start by opening a terminal in the root of this `/rnbo.unity.audioplugin` directory. Then run:

```sh
mkdir build
cd build
cmake .. -DPLUGIN_NAME="My Custom Plugin"
cmake --build .
```
If you change the name of your export from `rnbo_source.cpp` in RNBO's export sidebar, you can either change the Cmake variable `RNBO_CLASS_FILE_NAME` in `CMakeLists.txt` or use the Cmake flag `-DRNBO_CLASS_FILE_NAME` when you run `cmake ..`

For example:

```sh
mkdir build
cd build
cmake .. -DRNBO_CLASS_FILE_NAME="my_custom_plugin.cpp" -DPLUGIN_NAME="My Custom Plugin"
cmake --build .
```

After building, you should see a folder in your `/build` directory called **My Custom Plugin**. This should contain
all you need to install it as a package in Unity.


## Resources

* [rnbo](https://rnbo.cycling74.com/)
* [Unity](https://unity.com/)
  * [NativeAudioPlugins](https://github.com/Unity-Technologies/NativeAudioPlugins)
  * [NativeAudioPlugins Docs](https://docs.unity3d.com/Manual/AudioMixerNativeAudioPlugin.html)
  * [Package Manifest Docs](https://docs.unity3d.com/Manual/upm-manifestPkg.html)
  * [Assembly Definition docs](https://docs.unity3d.com/Manual/ScriptCompilationAssemblyDefinitionFiles.html)
* [cmake](https://cmake.org/)

## TODO

* Support additional platforms. Currently we only support MacOS, 64-bit Linux and 64-bit Windows.
* Async Preset Capture
