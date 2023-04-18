# Loading and Storing Presets

When you export a RNBO patch with [presets](https://rnbo.cycling74.com/learn/presets-with-snapshots), those presets will be exported next to your `description.json` in the `presets.json` file, and will be included in the Plugin that you build with this repository. 

## Loading a Preset

The `JSON` of each preset has been encoded as a string and stored in the Plugin Handle's `Presets` property. To find the content of a preset, you iterate through that property to find a match for the preset's name. The following example demonstrates how to load a preset called `"bright"` on a plugin called `FeedbackPolyphonyGroup`.

```c#
using UnityEngine;
using System.Linq;

public class PresetLoad : MonoBehaviour
{
    FeedbackPolyphonyGroupHelper helper;

    const int instanceIndex = 1;
    
    void Start()
    {
        helper = FeedbackPolyphonyGroupHelper.FindById(instanceIndex);
        
        var preset = FeedbackPolyphonyGroupHandle.Presets.First(x => x.name == "bright");
        if (preset != null) helper.Plugin.LoadPreset(preset.preset);
    }
}
```

## Storing a Preset

It is currently only possible to create a new preset synchronously, meaning you might get clicks or other disruptions to your audio while storing a new preset. For that reason, we don't recommend using this method as part of your live game, but rather only as a tool you might use during development. 

The following example sets new values for this plugin's `"overblow"` and `"harmonics"` parameters when the user presses the `"T"` key, and then stores the state of the device as a new preset when the user presses `"Y"`.

```c#
using UnityEngine;
using System.Linq;

public class PresetCreate : MonoBehaviour
{
    FeedbackPolyphonyGroupHelper helper;

    readonly System.Int32 harmonics = (int)FeedbackPolyphonyGroupHandle.GetParamIndexById("harmonics");
    readonly System.Int32 overblow = (int)FeedbackPolyphonyGroupHandle.GetParamIndexById("overblow");

    [SerializeField] double overblowParam;
    [SerializeField] double harmonicsParam;

    const int instanceIndex = 1;

    string newPreset;
    
    void Start()
    {
        helper = FeedbackPolyphonyGroupHelper.FindById(instanceIndex);
    }

    void Update()
    {
        // change the state of the RNBO device
        if (Input.GetKeyDown(KeyCode.T))
        {
            helper.Plugin.SetParamValue(overblow, overblowParam);
            helper.Plugin.SetParamValue(harmonics, harmonicsParam);
        }

        // store the current state of the RNBO device as a new preset
        if (Input.GetKeyDown(KeyCode.Y)) helper.Plugin.GetPresetSync(out newPreset);
    }
}

```

- Next: [Sending MIDI Messages](MIDI.md)
- Back to the [Table of Contents](README.md#table-of-contents)