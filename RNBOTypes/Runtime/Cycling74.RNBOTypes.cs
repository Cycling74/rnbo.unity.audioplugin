// Copyright (c) 2023 Cycling '74
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;
using UnityEngine;

namespace Cycling74.RNBOTypes {

    //TODO: alter based on define
    using Float = System.Double;
    using MillisecondTime = System.Double;
    using ParameterValue = System.Double;
    using ParameterIndex = System.UIntPtr;
    using MessageTag = System.UInt32;

    public class MIDIHeaders {
        public const byte NOTE_OFF = 0x80;
        public const byte NOTE_ON = 0x90;
        public const byte KEY_PRESSURE = 0xA0;
        public const byte CONTROL_CHANGE = 0xB0;
        public const byte PITCH_BEND_CHANGE = 0xE0;
        public const byte SONG_POSITION_POINTER = 0xF2;
        public const byte PROGRAM_CHANGE = 0xC0;
        public const byte CHANNEL_PRESSURE = 0xD0;
        public const byte QUARTER_FRAME = 0xF1;
        public const byte SONG_SELECT = 0xF3;
        public const byte TUNE_REQUEST = 0xF6;
        public const byte TIMING_CLOCK = 0xF8;
        public const byte START = 0xFA;
        public const byte CONTINUE = 0xFB;
        public const byte STOP = 0xFC;
        public const byte ACTIVE_SENSE = 0xFE;
        public const byte RESET = 0xFF;
        public const byte SYSEX_START = 0xF0;
        public const byte SYSEX_END = 0xF7;
    }

    public enum MessageEventType {
        Number,
        List,
        Bang
    }

    // Unity doesn't support JsonDocument (yet) so we work around it by specifying our own object with the keys we care about deserializing
    [System.Serializable]
    public class ParameterInfo {
        public int index;

        public string name;
        public string paramId;
        public string displayName;
        public string unit;

        public Float minimum;
        public Float maximum;
        public Float initialValue;
        public int steps;

        public List<string> enumValues;
        public string meta;
    }

    [System.Serializable]
    public class Port {
        public string tag;
        public string meta;
    }

    [System.Serializable]
    public class DataRef {
        public string id;
        public string type;
        public string file;
    }

    [System.Serializable]
    public class PatcherDescription {
        public int numParameters;
        public List<ParameterInfo> parameters;
        public List<Port> inports;
        public List<Port> outports;
        public List<DataRef> externalDataRefs;
    }

    public class ParameterChangedEventArgs : EventArgs {
        public ParameterChangedEventArgs(int index, ParameterValue value, MillisecondTime time)
        {
            Index = index;
            Value = value;
            Time = time;
        }

        public int Index { get; private set; }
        public ParameterValue Value { get; private set; }
        public MillisecondTime Time { get; private set; }
    }

    public class MessageEventArgs : EventArgs {
        public MessageEventArgs(MessageTag tag, MessageEventType messageType, Float[] values, MillisecondTime time)
        {
            Tag = tag;
            Type = messageType;
            Values = values;
            Time = time;
        }

        public MessageTag Tag { get; private set; }
        public MessageEventType Type { get; private set; }
        public Float[] Values { get; private set; }
        public MillisecondTime Time { get; private set; }
    }

    public class TempoEventArgs : EventArgs {
        public TempoEventArgs(Float val, MillisecondTime time)
        {
            Tempo = val;
            Time = time;
        }

        public Float Tempo { get; private set; }
        public MillisecondTime Time { get; private set; }
    }

    public class BeatTimeEventArgs : EventArgs {
        public BeatTimeEventArgs(Float val, MillisecondTime time)
        {
            BeatTime = val;
            Time = time;
        }

        public Float BeatTime { get; private set; }
        public MillisecondTime Time { get; private set; }
    }

    public class TransportEventArgs : EventArgs {
        public TransportEventArgs(bool running, MillisecondTime time)
        {
            Running = running;
            Time = time;
        }

        public bool Running { get; private set; }
        public MillisecondTime Time { get; private set; }
    }

    public class TimeSignatureEventArgs : EventArgs {
        public TimeSignatureEventArgs(int numerator, int denominator, MillisecondTime time)
        {
            Numerator = numerator;
            Denominator = denominator;
            Time = time;
        }

        public int Numerator { get; private set; }
        public int Denominator { get; private set; }
        public MillisecondTime Time { get; private set; }
    }

    public class PresetEventArgs : EventArgs {
        public PresetEventArgs(string payload)
        {
            Preset = payload;
        }

        public string Preset { get; private set; }
    }

    [System.Serializable]
    public class PresetEntry {
        public string name;
        //is actually json but encoded as a string so we don't have to parse it
        public string preset;
    }

    [System.Serializable]
    public class PresetList {
        public List<PresetEntry> presets;
    }

    public delegate void TransportRequestDelegate(IntPtr userData, MillisecondTime time, out byte running, out Float bpm, out Float beatTime, out int timeSigNum, out int timeSigDenom);

    public class Transport {

        public static Transport Global { get; } = new Transport();

        bool _running = true;
        Float _tempo = 100.0;
        Float _beatTime = 0.0;
        //using a single 32 bit number to represent 2 16bit numbers, so we can be atomic
        //would use UInt32 but the version of .net or whatever it is doesn't support it 
        Int32 _timeSignature = 4 << 16 | 4;

        public bool Running {
            get => _running;
            set => _running = value;
        }

        //there is no Interlocked.Load but CompareExchange returns the value so we simply compare against zero and then set to zero
        public Float Tempo { 
            get => Interlocked.CompareExchange(ref _tempo, 0.0, 0.0); 
            set {
                if (value < 0.0) {
                    throw new ArgumentOutOfRangeException("Tempo can only be positive");
                }
                Interlocked.Exchange(ref _tempo, value);
            }
        }
        public Float BeatTime { 
            get => Interlocked.CompareExchange(ref _beatTime, 0.0, 0.0); 
            set {
                if (value < 0.0) {
                    throw new ArgumentOutOfRangeException("BeatTime can only be positive");
                }
                Interlocked.Exchange(ref _beatTime, value);
            }
        }
        public (UInt16, UInt16) TimeSignature { 
            get {
                var cur = unchecked((UInt32)_timeSignature); //supposedly this is already atomic
                return ((UInt16)(cur >> 16), (UInt16)(cur & 0xFFFF));
            }
            set {
                if (value.Item1 == 1 || value.Item2 == 0) {
                    throw new ArgumentOutOfRangeException("Both Numerator and Denominator of TimeSignature must be non-zero");
                }
                UInt32 v = unchecked((UInt32)(value.Item1) << 16 | (UInt32)(value.Item2));
                Interlocked.Exchange(ref _timeSignature, (Int32)v);
            }
        }

        MillisecondTime _lastUpdate = -1.0;

        Float _seekTo = -1.0;
        public void SeekTo(Float beatTime) {
            Interlocked.Exchange(ref _seekTo, beatTime < 0.0 ? 0.0 : beatTime);
        }

        //only to be accessed from audio thread
        bool runningCur = true;
        Float tempoCur = 100.0;
        Float beatTimeCur = 0.0;
        (UInt16, UInt16) timeSignatureCur = (4, 4);

        [AOT.MonoPInvokeCallback(typeof(TransportRequestDelegate))]
        public static void AudioThreadUpdate(IntPtr inst, MillisecondTime time, out byte run, out Float tempo, out Float beatTime, out int timeSigNum, out int timeSigDenom) {
            GCHandle gch = GCHandle.FromIntPtr(inst);
            Transport transport = (Transport)gch.Target;
            if (transport != null) {
                transport.Update(time, out run, out tempo, out beatTime, out timeSigNum, out timeSigDenom);
            } else {
                run = 0; //false
                tempo = 100.0;
                beatTime = 0.0;
                timeSigNum  = 4;
                timeSigDenom = 4;
            }
        }
        
        internal void Update(MillisecondTime time, out byte run, out Float tempo, out Float beatTime, out int timeSigNum, out int timeSigDenom) {
            if (time != _lastUpdate) {
                var last = _lastUpdate;

                _lastUpdate = time;

                runningCur = Running;
                tempoCur = Tempo;
                timeSignatureCur = TimeSignature;

                var seek = Interlocked.Exchange(ref _seekTo, -1.0);
                if (seek >= 0.0) {
                    beatTimeCur = seek;
                } else {
                    //advance beat time
                    beatTimeCur = BeatTime;

                    MillisecondTime offset = time - Math.Max(0.0, last);
                    //it is an error if offset is negative
                    if (offset > 0.0) {
                        //mstobeats from rnbo
                        beatTimeCur += offset * tempoCur * 0.008 / 480.0;
                    }
                }
                BeatTime = beatTimeCur;
            }

            run = (byte)(runningCur ? 1 : 0);
            tempo = tempoCur;
            beatTime = beatTimeCur;
            timeSigNum = (int)timeSignatureCur.Item1;
            timeSigDenom = (int)timeSignatureCur.Item2;
        }
    }
}
