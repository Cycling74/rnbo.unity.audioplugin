# Working with RNBO in Unity

## Getting started

Over in your Unity project, in `/Assets`, create a `/Plugins` folder and drop in your `FooBar.bundle` from the `FooBar` package. In the Inspector for the Plugin, check "Load on Startup" and then "Apply."

## Where you can load your RNBO Device

### On an Audio Mixer

Create a new Audio Mixer and add the RNBO plugin to a track as an effect—for example, the "Master" track.

To hear your audio plugin in Unity, add an Audio Source Game Object and set its Output to the Master track of this Audio Mixer. 

### As an Audio Filter

## Getting and Setting Parameters

## Sending and Receiving Messages (Inlets + Outlets)

## Sending and Receiving MIDI 

## Events related to Musical Time

## Loading and Storing Presets

## Buffers and File Dependencies 

## Working with Multiple RNBO Devices

