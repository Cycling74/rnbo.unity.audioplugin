# Events related to Musical Time

The plugin handle includes methods for scheduling events related to musical time and a transport that is active in your RNBO device. 

The following example demonstrates how you could:

- Set the tempo to 60 bpm,
- Set the device's time signature to simple triple (3/4) time,
- Set the transport to be running (as opposed to stopped), and
- Schedule a `BeatTimeEvent` that moves the transport to a new time, specified in a number of quarter notes (in this case, 4 quarter notes) from the transport's start. This start time, or time 0, would correspond to 1.1.1 in Ableton Live's transport, for example.

```c#
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

The `BeatTimeEvent` can also be an effective means to synchronize multiple RNBO plugins — schedule such an event that sets your plugins to the same BeatTime, and, given that the plugins also share a Tempo and Time Signature, they should be in sync.

- Next: [Loading and Storing Presets](PRESETS.md)
- Back to the [Table of Contents](README.md#table-of-contents)