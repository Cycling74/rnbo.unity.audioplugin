## Addressing your RNBO Plugin from a C# Script

In the package built for your plugin, there is a `/Scripts` directory which contains a helper script which shares your plugin's name. For example, if we have a plugin called `QuantizedBuffers`, then we also have a helper script called `QuantizedBuffersHelper.cs`.

This helper script exposes a public `PLUGIN_NAMEHandle` class, or `QuantizedBuffersHandle`, in our case, which, along with the `PLUGIN_NAMEHelper` class, or in our case `QuantizedBuffersHelper`, helps us keep a *handle* on the instance of our plugin owned by the Audio Mixer. Let's take a look at how this might look in C#.

In the next few examples, we'll build a drum kit we can play in Unity using this `QuantizedBuffers` plugin. If you want to follow along yourself, you can find `quantized-buffers.maxpat` in [/docs/example-patches](./example-patches/).

In your Unity Project, in `Assets/Scripts/`, create a new C# script. The example below is called `DrumKit.cs`.

```c#
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

Then, in our `Start()` method, we access the helper's `Plugin` member, which is a `QuantizedBuffersHandle`, in order to gain access to methods of the handle that can get or set parameters, send and receive messages, and otherwise interact with the RNBO device.

- Next: [Getting and Setting Parameters](PARAMETERS.md)
- Back to the [Table of Contents](README.md#table-of-contents)