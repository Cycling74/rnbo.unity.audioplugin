# Events related to Musical Time

The plugin handle includes methods for scheduling events related to musical time and a transport that is active in your RNBO device. 

The following example demonstrates how you could:

- Set the tempo to 60 bpm,
- Set the device's time signature to simple triple (3/4) time,
- Set the transport to be running (as opposed to stopped), and
- Schedule a `BeatTimeEvent` that moves the transport to a new time, specified in a number of quarter notes (in this case, 4 quarter notes) from the transport's start. This start time, or time 0, would correspond to 1.1.1 in Ableton Live's transport, for example.

```csharp
using UnityEngine;

public class DrumKit : MonoBehaviour
{
    QuantizedBuffersHelper quantizedBuffersHelper;
    QuantizedBuffersHandle myQuantizedBuffersPlugin;

    const int instanceIndex = 1;

    [SerializeField] float tempo = 60;
    [SerializeField] int timeSignatureNumerator = 3;
    [SerializeField] int timeSignatureDenominator = 4;
    [SerializeField] float beatTime = 4f;
    [SerializeField] bool transportOn = true;

    void Start ()
    {
        quantizedBuffersHelper = QuantizedBuffersHelper.FindById(instanceIndex);
        myQuantizedBuffersPlugin = quantizedBuffersHelper.Plugin;

        myQuantizedBuffersPlugin.SetTransportRunning(transportOn);
        myQuantizedBuffersPlugin.SetTempo(tempo);
        myQuantizedBuffersPlugin.SetTimeSignature(timeSignatureNumerator, timeSignatureDenominator);
        myQuantizedBuffersPlugin.SetBeatTime(beatTime);
    }
}
```

## Synchronizing multiple RNBO plugins

You can synchronize multiple plugins having them share a `Transport`, which you can either set globally per plugin export type, or per instance.

So, for instance, if you wanted to synchronize instances of `QuantizedBuffers` plugins to a global transport, potentially shared by other plugins, you could do this:

```csharp
using UnityEngine;
using Cycling74.RNBOTypes;

public class DrumKit : MonoBehaviour
{
    QuantizedBuffersHelper quantizedBuffersHelper;
    QuantizedBuffersHandle myQuantizedBuffersPlugin;

    const int instanceIndex = 1;

    void Start ()
    {
        quantizedBuffersHelper = QuantizedBuffersHelper.FindById(instanceIndex);
        myQuantizedBuffersPlugin = quantizedBuffersHelper.Plugin;

        QuantizedBuffersHandle.RegisterGlobalTransport(Transport.Global);
    }

    void Update()
    {

        if (Input.GetKeyDown(KeyCode.U))
        {
            Transport.Global.Tempo += 2.0;
        }
    }
}
```

But if you can also set a specific instance to have a separate transport:

```csharp
using UnityEngine;
using Cycling74.RNBOTypes;

public class Foo : MonoBehaviour
{
    QuantizedBuffersHelper quantizedBuffersHelper;
    QuantizedBuffersHandle myQuantizedBuffersPlugin;
    Transport transport;

    const int instanceIndex = 2;

    void Start ()
    {
        transport = new Transport();
        transport.Tempo = 200.0;

        quantizedBuffersHelper = QuantizedBuffersHelper.FindById(instanceIndex);
        myQuantizedBuffersPlugin = quantizedBuffersHelper.Plugin;

        myQuantizedBuffersPlugin.RegisterTransport(transport);
    }

    void Update()
    {

        if (Input.GetKeyDown(KeyCode.U))
        {
            transport.Tempo += 2.0;
        }
    }
}
```

If you have both of these scripts loaded in your project, and 2 or more `QuantizedBuffers` plugins in your mixer,
you should have the one with `instanceIndex` 2 synced to a separate transport.
Any other instance would be synchronized to `Transport.Global`, which is a default, global, transport that you can
use if you want "global" synchronization.

If you want to remove transport synchronization, you can send `null` to the `RegisterTransport` and or `RegisterGlobalTransport`.


- Next: [Loading and Storing Presets](PRESETS.md)
- Back to the [Table of Contents](README.md#table-of-contents)
