# Making a Custom Filter with RNBO

```c#
using System;
using UnityEngine;
using UnityEngine.Audio;

[RequireComponent(typeof(AudioSource))]
public class PlayerMovement : MonoBehaviour
{
    SimpleFreqParamHandle synth;

    public PlayerMovement() : base() { }

    // Start is called before the first frame update
    void Start()
    {
        synth = new SimpleFreqParamHandle();
    }

    // Update is called once per frame
    void Update()
    {
        synth.Update();

        float horizontal = Input.GetAxisRaw("Horizontal");
        float vertical = Input.GetAxisRaw("Vertical");

        Vector3 direction = new Vector3(horizontal, 0f, vertical).normalized;
        transform.Translate(direction * speed * Time.deltaTime);
    }

    void OnAudioFilterRead(float[] data, int channels)
    {
        if (synth != null) {
            synth.Process(data, channels);
        }
    }

}
```

- Back to the [Table of Contents](RNBO_IN_UNITY.md#table-of-contents)