# Working with RNBO in Unity

The RNBO Unity Audio Plugin provides an API that facilitates integration of your RNBO device with the Unity game engine. Alongside the plugin itself, we build a helper object which contains various methods to help you access your device's parameters, presets, buffers, inlets and outlets.

In this guide, we'll discuss the basics of adding your RNBO device as a plugin in a Unity project and how to use this API to add procedural audio to your game. 

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

## Where you can load your RNBO Device

### On an Audio Mixer

Create a new Audio Mixer and add the RNBO plugin to a track as an effect—for example, the "Master" track.

To hear your audio plugin in Unity, add an Audio Source Game Object and set its Output to the Master track of this Audio Mixer.

*Note that when a plugin is loaded on an Audio Mixer, it will by default create a GUI in the Inspector with sliders for each parameter. These sliders are not necessarily functional, nor do they accurately represent the current value of a parameter in your RNBO device, especially if you are setting those parameters via a C# script.*

### As an Audio Filter

## Getting and Setting Parameters

## Sending and Receiving Messages (Inlets + Outlets)

## Sending and Receiving MIDI 

## Events related to Musical Time

## Loading and Storing Presets

## Buffers and File Dependencies 

## Working with Multiple RNBO Devices

