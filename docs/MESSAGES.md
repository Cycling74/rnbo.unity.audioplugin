# Sending and Receiving Messages

You can send and receive messages to and from your RNBO device via message tags defined by the `inport`, and `outport` objects in your RNBO patcher. 

## Sending a Messsage

This example sends a message into an inport called `"playSampleOne"`, which plays some audio from a buffer. For a description of how to load an audio file into a buffer, see [BUFFERS.md](BUFFERS.md).

```c#
using UnityEngine;

public class DrumKit : MonoBehaviour
{
    QuantizedBuffersHelper quantizedBuffersHelper;
    QuantizedBuffersHandle myQuantizedBuffersPlugin;

    const int instanceIndex = 1;

    [SerializeField] AudioClip buffer;

    readonly System.UInt32 inport = QuantizedBuffersHandle.Tag("playSampleOne");

    void Start ()
    {

        quantizedBuffersHelper = QuantizedBuffersHelper.FindById(instanceIndex);
        myQuantizedBuffersPlugin = quantizedBuffersHelper.Plugin;

        if (buffer)
          {
              float[] samples = new float[buffer.samples * buffer.channels];
              buffer.GetData(samples, 0);
              myQuantizedBuffersPlugin.LoadDataRef("sampleOne", samples, buffer.channels, buffer.frequency);
          }

        myQuantizedBuffersPlugin.SendMessage(inport, 1);
    }

}
```
