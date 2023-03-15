# RNBO Unity Audio Plugin

This is experimental software, use at your own risk.
Here we implement a Native Audio Plugin for Unity as well as a helper object for working around some of the limitations of that API.

After following the steps below, learn more about working with [RNBO in Unity](docs/GUIDE.md).

## Project organization

There are 2 pieces to this project.

The first piece is our **RNBOTypes** package. This contains C# code that your custom plugin 
export will reference. If you create multiple exports, they will all reference the same **RNBOTypes** 
project.

The second piece is our NativeAudioPlugin implementation and helper script.  This contains the
adapter code you need to convert your RNBO C++ export into a plugin that you can load in Unity.

### Building

We use cmake, here is a quick/dirty example. Assuming you've exported your patcher to a subdirectory
called `export` next to the `src` directory, and your export is called `rnbo_source.cpp`.

```sh
mkdir build
cd build
cmake .. -DPLUGIN_NAME="Foo Bar"
cmake --build .
```

After that you should see a folder in your build directory called **Foo Bar**. This should contain
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
