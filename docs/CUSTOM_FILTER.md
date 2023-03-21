# Making a Custom Filter with RNBO

An example script, which, given a plugin called **TestOrbs**, will instantiate a custom filter on a `GameObject` when attached as a component.

More documentation coming soon!

```c#
using UnityEngine;

[RequireComponent(typeof(AudioSource))]
public class OrbPlayer : MonoBehaviour
{
    TestOrbsHandle synth;

    public OrbPlayer() : base() {}
    
    void Start()
    {
        synth = new TestOrbsHandle();
    }

    void Update()
    {
        synth.Update();

    }

    void OnAudioFilterRead(float[] data, int channels) 
    {
        if (synth != null)
        {
            synth.Process(data, channels);
        }    
    }
}

```

- Back to the [Table of Contents](RNBO_IN_UNITY.md#table-of-contents)