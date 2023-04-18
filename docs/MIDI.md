# Sending MIDI Messages

You can send MIDI Note On, Note Off, and CC messages into your RNBO Plugin, and use MIDI to take advantage of RNBO's default [polyphonic voice management](https://rnbo.cycling74.com/learn/polyphony-and-voice-management-in-rnbo) in Unity.

The following example, which uses a Plugin called `FeedbackPolyphonyGroup`, schedules a note on and note off event for a random pitch in the range of MIDI note `40` to `100` whenever the user presses the `"Q"` key. It also sends the MIDI controller number `1` the value `110` when the script is first loaded.

```c#
using UnityEngine;

public class SendMIDI : MonoBehaviour
{
    FeedbackPolyphonyGroupHelper helper;

    const int instanceIndex = 1;
    
    void Start()
    {
        helper = FeedbackPolyphonyGroupHelper.FindById(instanceIndex);

        // MIDI channel 1, controller number 1, value 110
        helper.Plugin.SendMIDICC(1, 1, 110);
    }

    void Update()
    {
        if (Input.GetKeyDown(KeyCode.Q))
        {
            byte pitch = (byte)Random.Range(40, 100);

            // uses the Plugin's `Now` property to define a time to send the MIDI note on message
            double startTime = helper.Plugin.Now + 0.01;

            // MIDI channel 1, sends `pitch` with a velocity of 127 at the `startTime` defined above
            helper.Plugin.SendMIDINoteOn(1, pitch, 127, startTime);
            // schedules the note off event for 200ms after the note on event
            helper.Plugin.SendMIDINoteOff(1, pitch, 0, startTime + 200);
        }
    }
}

```

- Next: [Making a Custom Filter](CUSTOM_FILTER.md)
- Back to the [Table of Contents](README.md#table-of-contents)