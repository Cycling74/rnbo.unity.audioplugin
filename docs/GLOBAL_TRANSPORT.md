# Synchronizing multiple RNBO plugins

## Using the Global Transport

You can synchronize multiple plugins having them share a `Transport`, which you can either set globally per plugin export type, or per instance.

For instance, if you wanted to synchronize instances of `QuantizedBuffers` plugins to a global transport, potentially shared by other plugins, you could use `.RegisterGlobalTransport()`, like this:

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

## Set up a seperate, local Transport

You can also set a specific instance to have a separate transport:

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

If you have both of these scripts loaded in your project, and 2 or more `QuantizedBuffers` plugins in your mixer, the one with `instanceIndex` of `2` should now be synced to a separate transport.
Any other instance would be synchronized to `Transport.Global`, which is a default, global, transport that you can
use if you want "global" synchronization.

## Removing Transport synchronization

If you want to remove transport synchronization, you can send `null` to the `RegisterTransport` and or `RegisterGlobalTransport`.

- Next: [Loading and Storing Presets](PRESETS.md)
- Back to the [Table of Contents](INDEX.md)
