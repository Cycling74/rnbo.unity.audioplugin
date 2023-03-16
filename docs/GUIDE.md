# Working with RNBO in Unity

The RNBO Unity Audio Plugin provides an API that facilitates integration of your RNBO device with the Unity game engine. Alongside the plugin itself, we build a helper object which contains various methods to help you access your device's parameters, presets, buffers, inlets and outlets.

In this guide, we'll discuss the basics of adding your RNBO device as a plugin in a Unity project and how to use this API to add procedural audio to your game. As this integration is in an expiremental state, so is this documentation, and this guide should not be considered an encyclopedic reference, but rather a quick-start with a handful of ways to use this tool.

## Getting started

After you've followed the steps [in the README](../README.md) to build the package that contains your RNBO plugin, you can install that package, and the **RNBOTypes** package, using Unity's Package Manager. 

### Installation

You should use "Add package from disk..." via Unity's Package Manager to install both the **RNBOTypes** and the package containing your plugin.

![add package from disk](images/add-package-from-disk.jpg)

In the dialog box that opens, select the package directory, and choose the file `package.json` to Open. 

***NB:** Please be sure to move the package you've built out of the `/build` directory and into some place safe on your machine. Installing the package will not necessarily copy its files into your Unity Project. If you delete your `/build` directory with the package still inside, your Unity Project will lose access to the package as well.*

This should result in the **RNBO Unity Types** package and the package named with the name you chose for your RNBO plugin added to the Installed Packages for your project. 

![packages in project window](images/packages-in-project-window.png)

Inside the package for your plugin, there should be a directory called `/Assets` which includes your plugin in `Plugins` and the helper object in `Scripts`.

Find your plugin, which on macOS will be a `.bundle`, on Windows a `.dll`, and on Linux a `.so`, and select it so that it will open in Unity's Inspector.

In the Inspector, check "Load on Startup" and then "Apply."

![load-on-startup](images/load-on-startup.png)

Now you you should be able to load your RNBO plugin in your Unity project.

## Where you can load your RNBO Plugin

There are ways to add your RNBO plugin into your project—as a plugin loaded onto a track on an Audio Mixer, or by implementing a custom filter in a script attached as a Component to a GameObject. 

In this document, we'll discuss loading the plugin on an Audio Mixer. When you create a custom filter, you'll use many of the same methods to access your RNBO device, but the means of setting up the filter are distinct enough to warrant a seperate document. Visit [CUSTOMFILTER.md](CUSTOMFILTER.md) for that guide.

### Loading your Plugin on an Audio Mixer

Create a new [Audio Mixer](https://docs.unity3d.com/Manual/AudioMixer.html) and add the RNBO plugin to a track as an effect—for example, the "Master" track—from the "Add..." dropdown menu.

![instance-index](images/instance-index.png)

Once you've added your plugin, set an instance index — this should be an **integer** that you will use in your scripts to refer to a specific *instance* of your plugin, loaded on this Audio Mixer. Note that although Unity's default GUI indicates you should be able to set a *float* value here, you should make sure to use a whole number. 

To hear your audio plugin in Unity, add an [Audio Source](https://docs.unity3d.com/Manual/class-AudioSource.html) and set its **Output** to the Master track of this Audio Mixer.

*Note that when a plugin is loaded on an Audio Mixer, it will by default create a GUI in the Inspector with sliders for each parameter. These sliders are not necessarily functional, nor do they accurately represent the current value of a parameter in your RNBO device, especially if you are setting those parameters via a C# script.*

## Addressing your RNBO Plugin from a C# Script

In the package built for your plugin, there is a `/Scripts` directory which contains a helper script which shares your plugin's name. For example, if we have a plugin called `QuantizedBuffers`, then we also have a helper script called `QuantizedBuffersHelper.cs`.

This helper script exposes a public `PLUGIN_NAMEHandle` class, or `QuantizedBuffersHandle`, in our case, which, along with the `PLUGIN_NAMEHelper` class, or in our case `QuantizedBuffersHelper`, helps us keep a *handle* on the instance of our plugin owned by the Audio Mixer. Let's take a look at how this might look in C#.

In the next few examples, we'll build a drum kit we can play in Unity using this `QuantizedBuffers` plugin. If you want to follow along yourself, you can find `quantized-buffers.maxpat` in `/docs/example-patches`.

In your Unity Project, in `Assets/Scripts/`, create a new C# script. The example below is called `DrumKit.cs`.

```csharp
using UnityEngine

public class DrumKit : MonoBehaviour
{
    QuantizedBuffersHelper quantizedBuffersHelper;
    QuantizedBuffersHandle myQuantizedBuffersPlugin;

    const int instanceIndex = 1; // this corresponds to the Instance Index key we set in the mixer

    void Start()
    {
        quantizedBuffersHelper = QuantizedBuffersHelper.FindById(instanceIndex);
        myQuantizedBuffersPlugin = quantizedBuffersHelper.Plugin;
    }
}
```

Here, we create a class property of type `QuantizedBuffersHelper` and another of type `QuantizedBuffersHandle`. We'll use the first to ensure that our script targets the specific instance of our plugin that we want. 

We then access the helper's `Plugin` member, which is a `QuantizedBuffersHandle` in order to gain access to methods of the handle that can get or set parameters, send and receive messages, and otherwise interact with the RNBO device.

## Getting and Setting Parameters



## Sending and Receiving Messages (Inlets + Outlets)

## Sending and Receiving MIDI 

## Events related to Musical Time

## Loading and Storing Presets

## Buffers and File Dependencies 

## Working with Multiple RNBO Devices

