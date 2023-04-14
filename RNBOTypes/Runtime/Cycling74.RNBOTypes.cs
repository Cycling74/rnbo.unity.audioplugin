// Copyright (c) 2023 Cycling '74
using System;
using System.Collections.Generic;
using System.Threading;

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

    public struct TransportChanges {
        public bool RunningChanged { get; private set; } = false;
        public bool TempoChanged { get; private set; } = false;
        public bool BeatTimeChanged { get; private set; } = false;
        public bool TimeSignatureChanged { get; private set; } = false;

        bool _running;
        Float _tempo;
        Float _beatTime;
        (int, int) _timeSignature;

        TransportChanges() {
            _running = false;
            _tempo  = 0.0;
            _beatTime = 0.0;
            _timeSignature = (4, 4);
        }
        
        public bool Running {
            get => _running; 
            internal set {
                if (value != _running) {
                    _running = value;
                    RunningChanged = true;
                } else {
                    RunningChanged = false;
                }
            }
        }

        public Float Tempo {
            get => _tempo; 
            internal set {
                if (value != _tempo) {
                    _tempo = value;
                    TempoChanged = true;
                } else {
                    TempoChanged = false;
                }
            }
        }

        public Float BeatTime {
            get => _beatTime; 
            internal set {
                if (value != _beatTime) {
                    _beatTime = value;
                    BeatTimeChanged = true;
                } else {
                    BeatTimeChanged = false;
                }
            }
        }

        public (int, int) TimeSignature {
            get => _timeSignature; 
            internal set {
                if (value != _timeSignature) {
                    _timeSignature = value;
                    TimeSignatureChanged = true;
                } else {
                    TimeSignatureChanged = false;
                }
            }
        }
    }

    public class Transport {
        public bool Running { get; set; } = true;
        public Float Tempo { get; set; } = 100.0;
        public Float BeatTime { get; private set; } = 0.0;
        public (int, int) TimeSignature { get; set; } = (4, 4);

        double _lastUpdate = -1.0;
        public double? LastUpdate { 
            get => {
                var v = _lastUpdate;
                return v >= 0.0 ? v : null; 
            }
        }

        TransportChanges _changes = new TransportChanges();

        Float _seekTo = -1.0;
        public SeekTo(Float beatTime) {
            if (beatTime >= 0.0) {
                Interlocked.Exchange(_seekTo, beatTime);
            }
        }

        //update in the AudioThread
        public TransportChanges Update() {
            var now = AudioSettings.dspTime;
            if (now != _lastUpdate) {
                _lastUpdate = now;

                var running = Running;
                var tempo = Tempo;
                var beatTime = BeatTime;
                var sig = TimeSignature;

                var seek = Interlocked.Exchange(_seekTo, -1.0);
                if (seek >= 0.0) {
                    beatTime = seek;
                } else if (_lastUpdate >= 0.0) {
                    //advance beat time
                }
                //XXX do we need to interlock?
                BeatTime  = beatTime;

                _changes.Running = running;
                _changes.Tempo = tempo;
                _changes.BeatTime = beatTime;
                _changes.TimeSignature = sig;
            }
            return _changes;
        }

    }
}
