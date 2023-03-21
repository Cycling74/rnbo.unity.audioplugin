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

In the script above, we initialize a class property `inport`, an integer of type `System.UInt32`, which will hold the index of the inport called `"playSampleOne"` in the array of all inports from our explorted patch using the plugin handle's `.Tag()` method, which takes a string—the name of the inport in our RNBO patch.

We can then use that inport index to send a message using the `.SendMessage()` method, which we call on our `Plugin`.

## Subscribing to a Message Event

We can also subscribe to Message Events that come from our RNBO device. We need to use the `Cycling74.RNBOTypes` namespace, which contains the `MessageEventArgs` class. 

This script subscribes to a plugin's Message Events, gets the name of the Message Tag, and prints the message to the Debug console along with the time the message was received.

```c#
using UnityEngine;
using Cycling74.RNBOTypes;

public class DrumKit : MonoBehaviour
{
    QuantizedBuffersHelper quantizedBuffersHelper;
    QuantizedBuffersHandle myQuantizedBuffersPlugin;

    const int instanceIndex = 1;

    void Start()
    {
        quantizedBuffersHelper = QuantizedBuffersHelper.FindById(instanceIndex);
        myQuantizedBuffersPlugin = quantizedBuffersHelper.Plugin;
        
        myQuantizedBuffersPlugin.MessageEvent += (object sender, MessageEventArgs args) => {
            myQuantizedBuffersPlugin.ResolveTag(args.Tag, out string tagString);
            Debug.LogFormat("From outport {0}, received message [{1}] at {2} ms", tagString, string.Join(", ", args.Values), args.Time);
        };
    }
}
```

- Next: Loading and Storing Presets
- Back to the [Table of Contents](RNBO_IN_UNITY.md#table-of-contents)