#include <AudioPluginUtil.h>
#include <RNBO.h>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <limits>
#include <atomic>
#include <readerwriterqueue/readerwriterqueue.h>

#include <rnbo_description.h>
#include <iostream>

using RNBO::ParameterType;

//callbacks
extern "C" {
	typedef void (UNITY_AUDIODSP_CALLBACK * CParameterEventCallback)(size_t, RNBO::ParameterValue, RNBO::MillisecondTime);
	typedef void (UNITY_AUDIODSP_CALLBACK * CMessageEventCallback)(uint32_t, size_t, RNBO::ParameterValue *, size_t, RNBO::MillisecondTime);
	typedef void (UNITY_AUDIODSP_CALLBACK * CReleaseDataRefCallback)(size_t);
}

namespace RNBOUnity
{
	UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK CreateCallback            (UnityAudioEffectState* state);
	UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ReleaseCallback           (UnityAudioEffectState* state);
	UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ProcessCallback           (UnityAudioEffectState* state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int outchannels);
	UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK SetFloatParameterCallback (UnityAudioEffectState* state, int index, float value);
	UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK GetFloatParameterCallback (UnityAudioEffectState* state, int index, float* value, char *valuestr);
	UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK GetFloatBufferCallback    (UnityAudioEffectState* state, const char* name, float* buffer, int numsamples);
	int InternalRegisterEffectDefinition(UnityAudioEffectDefinition& definition);
}

extern "C" UNITY_AUDIODSP_EXPORT_API int AUDIO_CALLING_CONVENTION UnityGetAudioEffectDefinitions(UnityAudioEffectDefinition*** definitionptr)
{
    static UnityAudioEffectDefinition definition;
    static UnityAudioEffectDefinition* definitionp = &definition;
		UInt32 flags = 0;

#if PLUGIN_IS_SPATIALIZER==1
		flags |= UnityAudioEffectDefinitionFlags_IsSpatializer;
#endif

		AudioPluginUtil::DeclareEffect(
				definition,
				PLUGIN_NAME,
				flags,
				RNBOUnity::CreateCallback,
				RNBOUnity::ReleaseCallback,
				RNBOUnity::ProcessCallback,
				RNBOUnity::SetFloatParameterCallback,
				RNBOUnity::GetFloatParameterCallback,
				RNBOUnity::GetFloatBufferCallback,
				RNBOUnity::InternalRegisterEffectDefinition
				);

		*definitionptr = &definitionp;
    return 1;
}

namespace RNBOUnity
{
	class UnityEventHandler : public RNBO::EventHandler {
		public:
			typedef std::function<void(const RNBO::MessageEvent)> MessageEventCallback;
			typedef std::function<void(const RNBO::MidiEvent)> MidiEventCallback;
			typedef std::function<void(const RNBO::TransportEvent)> TransportEventCallback;
			typedef std::function<void(const RNBO::TempoEvent)> TempoEventCallback;
			typedef std::function<void(const RNBO::BeatTimeEvent)> BeatTimeEventCallback;
			typedef std::function<void(const RNBO::TimeSignatureEvent)> TimeSignatureEventCallback;
			typedef std::function<void(const RNBO::ParameterEvent)> ParameterEventCallback;
			typedef std::function<void()> PresetTouchedCallback;

			UnityEventHandler(
					MessageEventCallback mc = nullptr,
					MidiEventCallback mec = nullptr,

					TransportEventCallback transportCallback = nullptr,
					TempoEventCallback tempoCallback = nullptr,
					BeatTimeEventCallback beatTimeCallback = nullptr,
					TimeSignatureEventCallback timeSigCallback = nullptr,

					ParameterEventCallback pc = nullptr,
					PresetTouchedCallback pt = nullptr
					) :
				mEventsAvailable(false),
				mMessageEventCallback(mc),
				mMidiEventCallback(mec),

				mTransportEventCallback(transportCallback),
				mTempoEventCallback(tempoCallback),
				mBeatTimeEventCallback(beatTimeCallback),
				mTimeSignatureEventCallback(timeSigCallback),

				mParameterEventCallback(pc),
				mPresetTouchedCallback(pt)
				{
				}

			//only call from the poll thread
			void setMessageEventCallback(MessageEventCallback cb) { mMessageEventCallback = cb; };
			void setMidiEventCallback(MidiEventCallback cb) { mMidiEventCallback = cb; };
			void setTransportEventCallback(TransportEventCallback cb) { mTransportEventCallback = cb; };
			void setTempoEventCallback(TempoEventCallback cb) { mTempoEventCallback = cb; };
			void setBeatTimeEventCallback(BeatTimeEventCallback cb) { mBeatTimeEventCallback = cb; };
			void setTimeSignatureEventCallback(TimeSignatureEventCallback cb) { mTimeSignatureEventCallback = cb; };
			void setParameterEventCallback(ParameterEventCallback cb) { mParameterEventCallback = cb; };
			void setPresetTouchedCallback(PresetTouchedCallback cb) { mPresetTouchedCallback = cb; };

			void eventsAvailable() override {
				mEventsAvailable.store(true);
			}

			void poll() {
				bool expected = true;
				if (mEventsAvailable.compare_exchange_weak(expected, false)) {
					drainEvents();
				}
			}

			void handlePresetEvent(const RNBO::PresetEvent& pe) override {
				if (mPresetTouchedCallback && pe.getType() == RNBO::PresetEvent::Touched) {
					mPresetTouchedCallback();
				}
			}

			void handleParameterEvent(const RNBO::ParameterEvent& event) override {
				if (mParameterEventCallback)
					mParameterEventCallback(event);
			}

			void handleMessageEvent(const RNBO::MessageEvent& event) override {
				if (mMessageEventCallback)
					mMessageEventCallback(event);
			}

			void handleMidiEvent(const RNBO::MidiEvent& event) override {
				if (mMidiEventCallback)
					mMidiEventCallback(event);
			}

			void handleTransportEvent(const RNBO::TransportEvent& e) override
			{
				if (mTransportEventCallback)
					mTransportEventCallback(e);
			}

			void handleTempoEvent(const RNBO::TempoEvent& e) override
			{
				if (mTempoEventCallback)
					mTempoEventCallback(e);
			}

			void handleBeatTimeEvent(const RNBO::BeatTimeEvent& e) override
			{
				if (mBeatTimeEventCallback)
					mBeatTimeEventCallback(e);
			}

			void handleTimeSignatureEvent(const RNBO::TimeSignatureEvent& e) override
			{
				if (mTimeSignatureEventCallback)
					mTimeSignatureEventCallback(e);
			}

		private:
			std::atomic<bool> mEventsAvailable;

			MessageEventCallback mMessageEventCallback;
			MidiEventCallback mMidiEventCallback;
			TransportEventCallback mTransportEventCallback;
			TempoEventCallback mTempoEventCallback;
			BeatTimeEventCallback mBeatTimeEventCallback;
			TimeSignatureEventCallback mTimeSignatureEventCallback;

			ParameterEventCallback mParameterEventCallback;
			PresetTouchedCallback mPresetTouchedCallback;
	};

	const int32_t invalidKey = 0;
	const int32_t maxKey = 16777216;

	struct InnerData {
			UnityEventHandler mEventHandler;
			RNBO::CoreObject mCore;
			int32_t mInstanceKey = invalidKey;

			InnerData() : mCore(&mEventHandler) {}
	};

	//TODO do align
	struct EffectData {
		union
		{
			InnerData inner;
			//pad for ps3
			unsigned char pad[(sizeof(RNBO::CoreObject) + 15) & ~15];
		};
		EffectData() : inner() { }
		~EffectData() { }
	};

	std::vector<RNBO::ParameterIndex> param_index_map;

#if RNBO_UNITY_INSTANCE_ACCESS_HACK == 1
	std::shared_mutex instances_mutex;
	std::unordered_map<int32_t, EffectData *> instances;
#endif

	UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK CreateCallback(UnityAudioEffectState* state) {
		EffectData * effectdata = new EffectData();
		state->effectdata = effectdata;
		effectdata->inner.mCore.prepareToProcess(state->samplerate, state->dspbuffersize);
		return UNITY_AUDIODSP_OK;
	}

	UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ReleaseCallback(UnityAudioEffectState* state) {
		EffectData* effectdata = state->GetEffectData<EffectData>();

#if RNBO_UNITY_INSTANCE_ACCESS_HACK == 1
		{
			auto key = effectdata->inner.mInstanceKey;
			if (key != invalidKey) {
				std::unique_lock wlock(instances_mutex);
				instances.erase(key);
			}
		}
#endif

		delete effectdata;
		state->effectdata = nullptr;

		return UNITY_AUDIODSP_OK;
	}

	UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ProcessCallback(UnityAudioEffectState* state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int outchannels) {
		EffectData* effectdata = state->GetEffectData<EffectData>();
		if (state->flags & (UnityAudioEffectStateFlags_IsMuted | UnityAudioEffectStateFlags_IsPaused) || (state->flags & UnityAudioEffectStateFlags_IsPlaying) == 0) {
			memset(outbuffer, 0, length * outchannels * sizeof(float));
			return UNITY_AUDIODSP_OK;
		}

		effectdata->inner.mCore.process(inbuffer, inchannels, outbuffer, outchannels, length, nullptr, nullptr);

		return UNITY_AUDIODSP_OK;
	}

	UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK SetFloatParameterCallback(UnityAudioEffectState* state, int index, float value) {
		EffectData* effectdata = state->GetEffectData<EffectData>();

		//set index map for later retrieval
#if RNBO_UNITY_INSTANCE_ACCESS_HACK == 1
		if (index == 0) {
			std::unique_lock wlock(instances_mutex);
			int32_t key = static_cast<int32_t>(value);
			//integer precision
			assert(key >= 0 && key <= 16777216);
			if (key != invalidKey) {
				//test for collision
				auto it = instances.find(key);
				if (it != instances.end()) {
					it->second->inner.mInstanceKey = invalidKey;
				}
				instances[key] = effectdata;
			} else if (effectdata->inner.mInstanceKey != invalidKey) {
				instances.erase(effectdata->inner.mInstanceKey);
			}
			effectdata->inner.mInstanceKey = key;
			return UNITY_AUDIODSP_OK;
		}
#endif

		if (index < 0 || index >= param_index_map.size())
			return UNITY_AUDIODSP_ERR_UNSUPPORTED;
		auto mapped = param_index_map[index];
		effectdata->inner.mCore.setParameterValue(mapped, value);
		return UNITY_AUDIODSP_OK;
	}

	UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK GetFloatParameterCallback(UnityAudioEffectState* state, int index, float* value, char *valuestr) {
		EffectData* effectdata = state->GetEffectData<EffectData>();
		if (index < 0 || index >= param_index_map.size())
			return UNITY_AUDIODSP_ERR_UNSUPPORTED;

		if (valuestr != NULL)
			valuestr[0] = 0;

#if RNBO_UNITY_INSTANCE_ACCESS_HACK == 1
		if (index == 0) {
			if (value != NULL)
				*value = static_cast<float>(effectdata->inner.mInstanceKey);
			return UNITY_AUDIODSP_OK;
		}
#endif

		auto mapped = param_index_map[index];
		if (value != NULL)
			*value = static_cast<float>(effectdata->inner.mCore.getParameterValue(mapped));
		return UNITY_AUDIODSP_OK;
	}

	UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK GetFloatBufferCallback(UnityAudioEffectState* /* state */, const char* /* name */, float* /* buffer */, int /* numsamples */) {
		return UNITY_AUDIODSP_ERR_UNSUPPORTED;
	}

	int InternalRegisterEffectDefinition(UnityAudioEffectDefinition& definition) {
		RNBO::CoreObject core;
		if (param_index_map.size() == 0) {
#if RNBO_UNITY_INSTANCE_ACCESS_HACK == 1
			param_index_map.push_back(0); // the first index is always the instance index
#endif
			const RNBO::ParameterIndex numparams = core.getNumParameters();
			for (RNBO::ParameterIndex i = 0; i < numparams; i++) {
				RNBO::ParameterInfo info;
				core.getParameterInfo(i, &info);
				if (info.type != ParameterType::ParameterTypeNumber || !info.visible || info.debug)
					continue;
				param_index_map.push_back(i);
			}
		}

		definition.paramdefs = new UnityAudioParameterDefinition[param_index_map.size()];

		for (int i = 0; i < static_cast<int>(param_index_map.size()); i++) {
#if RNBO_UNITY_INSTANCE_ACCESS_HACK == 1
			if (i == 0) {
				AudioPluginUtil::RegisterParameter(definition, "Instance Index (edit with care)", "key", 0.0, static_cast<float>(maxKey), static_cast<float>(invalidKey), 1.0f, 1.0f, i);
				continue;
			}
#endif
			auto mapped = param_index_map[i];
			RNBO::ParameterInfo info;
			core.getParameterInfo(mapped, &info);
			std::string name = core.getParameterId(mapped);
			AudioPluginUtil::RegisterParameter(definition, name.c_str(), info.unit, info.min, info.max, info.initialValue, 1.0f, 1.0f, i);
		}
		return static_cast<int>(param_index_map.size());
	}
}

namespace {

#if RNBO_UNITY_INSTANCE_ACCESS_HACK == 1
	//RNBO calls release callback in the audio thread, so we queue an ID to tell the csharp side to release on Poll()
	std::mutex datarefReleaseQueueMutex; //only for reading from it
	moodycamel::ReaderWriterQueue<char *, 32> datarefReleaseQueue;

	void DataRefRelease(RNBO::ExternalDataId, char* d) {
		datarefReleaseQueue.try_enqueue(d);
	}

	bool with_instance(int32_t key, std::function<void(RNBOUnity::EffectData *)> func) {
		std::shared_lock rlock(RNBOUnity::instances_mutex);
		auto it = RNBOUnity::instances.find(key);
		if (it != RNBOUnity::instances.end()) {
			func(it->second);
			return true;
		}
		return false;
	}
#endif
}

//custom entrypoints

#if RNBO_UNITY_INSTANCE_ACCESS_HACK == 1
extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOInstanceMapped(int32_t key)
{
	return with_instance(key, [](RNBOUnity::EffectData*) { /*do nothing*/ });
}

extern "C" UNITY_AUDIODSP_EXPORT_API const char * AUDIO_CALLING_CONVENTION RNBOGetDescription()
{
	return RNBO::patcher_description.c_str();
}

extern "C" UNITY_AUDIODSP_EXPORT_API const char * AUDIO_CALLING_CONVENTION RNBOGetPresets()
{
	static std::mutex localmutex;
  static std::string presetsString;

  std::lock_guard guard(localmutex);

  //since we can't easily parse arbitrary JSON in unity yet, we simply convert the preset payloads
  //to strings
  if (presetsString.empty()) {
    nlohmann::json presets = nlohmann::json::array();
    try {
      nlohmann::json local = nlohmann::json::parse(RNBO::patcher_presets);

      if (local.is_array()) {
        for (auto p: local) {
          if (!(p.is_object() && p.contains("name") && p.contains("preset"))) {
            std::cerr << "unexpected preset entry format" << std::endl;
            continue;
          }

          std::string name = p["name"].get<std::string>();
          std::string payload = p["preset"].dump();

          nlohmann::json entry = {
              {"name", name},
              {"preset", payload}
          };

          presets.push_back(entry);
        }
      }
    } catch (std::exception& e) {
      std::cerr << "exception processing presets " << e.what() << std::endl;
    }

    //add presets prefix so it is easier for Unity side to parse
    nlohmann::json o = {
      {"presets", presets}
    };
    presetsString = o.dump();
  }

	return presetsString.c_str();
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOLoadPreset(int32_t key, const char * payload)
{
	return with_instance(key, [payload](RNBOUnity::EffectData* effectdata) {
      try {
        auto preset = RNBO::convertJSONToPreset(std::string(payload));
        effectdata->inner.mCore.setPreset(std::move(preset));
      } catch (std::exception& e) {
        std::cerr << "error converting preset payload to RNBO preset " << e.what() << std::endl;
      }
  });
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOPoll(int32_t key)
{
	//service the shared release queue
	{
		std::unique_lock<std::mutex> guard(datarefReleaseQueueMutex, std::try_to_lock);
		if (guard.owns_lock()) {
			char * d = nullptr;
			while (datarefReleaseQueue.try_dequeue(d)) {
				if (d) {
					delete [] d;
				}
			}
		}
	}

	return with_instance(key, [](RNBOUnity::EffectData* effectdata) {
			effectdata->inner.mEventHandler.poll();
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API RNBO::MessageTag AUDIO_CALLING_CONVENTION RNBOTag(const char * tagChar)
{
	return RNBO::TAG(tagChar);
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOResolveTag(int32_t key, RNBO::MessageTag tag, const char** tagChar)
{
	return with_instance(key, [tag, tagChar](RNBOUnity::EffectData* effectdata) {
			if (tagChar) {
				*tagChar = effectdata->inner.mCore.resolveTag(tag);
			}
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSetParamValue(int32_t key, RNBO::ParameterIndex index, RNBO::ParameterValue value, RNBO::MillisecondTime attime)
{
	return with_instance(key, [index, value, attime](RNBOUnity::EffectData* effectdata) {
			effectdata->inner.mCore.setParameterValue(index, value, attime);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOGetParamValue(int32_t key, RNBO::ParameterIndex index, RNBO::ParameterValue * valueOut)
{
	return with_instance(key, [index, valueOut](RNBOUnity::EffectData* effectdata) {
			if (valueOut != nullptr) {
				*valueOut = effectdata->inner.mCore.getParameterValue(index);
			}
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSetParamValueNormalized(int32_t key, RNBO::ParameterIndex index, RNBO::ParameterValue value, RNBO::MillisecondTime attime)
{
	return with_instance(key, [index, value, attime](RNBOUnity::EffectData* effectdata) {
			effectdata->inner.mCore.setParameterValueNormalized(index, value, attime);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOGetParamValueNormalized(int32_t key, RNBO::ParameterIndex index, RNBO::ParameterValue * valueOut)
{
	return with_instance(key, [index, valueOut](RNBOUnity::EffectData* effectdata) {
			if (valueOut != nullptr) {
				*valueOut = effectdata->inner.mCore.convertToNormalizedParameterValue(index, effectdata->inner.mCore.getParameterValue(index));
			}
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSendMessageNumber(int32_t key, RNBO::MessageTag tag, RNBO::number v, RNBO::MillisecondTime attime)
{
	return with_instance(key, [&tag, v, attime](RNBOUnity::EffectData* effectdata) {
			RNBO::MessageEvent event(tag, attime, v);
			effectdata->inner.mCore.scheduleEvent(event);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSendMessageBang(int32_t key, RNBO::MessageTag tag, RNBO::MillisecondTime attime)
{
	return with_instance(key, [tag, attime](RNBOUnity::EffectData* effectdata) {
			RNBO::MessageEvent event(tag, attime);
			effectdata->inner.mCore.scheduleEvent(event);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSendMessageList(int32_t key, RNBO::MessageTag tag, const RNBO::number* buffer, size_t bufferlen, RNBO::MillisecondTime attime)
{
	return with_instance(key, [tag, buffer, bufferlen, attime](RNBOUnity::EffectData* effectdata) {
		auto l = std::make_unique<RNBO::list>();
		for (auto i = 0; i < bufferlen; i++) {
			l->push(buffer[i]);
		}
		RNBO::MessageEvent event(tag, attime, std::move(l));
		effectdata->inner.mCore.scheduleEvent(event);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSendMIDI(int32_t key, const uint8_t* bytes, int len, RNBO::MillisecondTime attime)
{
	return with_instance(key, [bytes, len, attime](RNBOUnity::EffectData* effectdata) {
			RNBO::MidiEvent event(attime, 0, bytes, len);
			effectdata->inner.mCore.scheduleEvent(event);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOCopyLoadDataRef(int32_t key, const char * id, const float * data, size_t datalen, size_t channels, size_t samplerate)
{
	return with_instance(key, [id, data, datalen, channels, samplerate](RNBOUnity::EffectData* effectdata) {
			RNBO::Float32AudioBuffer bufferType(channels, static_cast<double>(samplerate));

			//we need to create our own copy as RNBO might write into the data AND it also might realloc
			float * d = new float[datalen];
			size_t bytes = sizeof(float) * datalen;
			std::memcpy(d, data, bytes);
			effectdata->inner.mCore.setExternalData(id, reinterpret_cast<char *>(d), bytes, bufferType, DataRefRelease);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOReleaseDataRef(int32_t key, const char * id)
{
	return with_instance(key, [id](RNBOUnity::EffectData* effectdata) {
			effectdata->inner.mCore.releaseExternalData(id);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBORegisterParameterEventCallback(int32_t key, CParameterEventCallback callback)
{
	return with_instance(key, [callback](RNBOUnity::EffectData* effectdata) {
			effectdata->inner.mEventHandler.setParameterEventCallback([callback](RNBO::ParameterEvent event) {
					if (callback) {
						callback(event.getIndex(), event.getValue(), event.getTime());
					}
			});
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBORegisterMessageEventCallback(int32_t key, CMessageEventCallback callback)
{
	return with_instance(key, [callback](RNBOUnity::EffectData* effectdata) {
			effectdata->inner.mEventHandler.setMessageEventCallback([callback](RNBO::MessageEvent event) {
					if (callback && event.getObjectId() == 0) {
						if (event.getType() == RNBO::MessageEvent::Type::List) {
							//hold shared pointer
							auto l = event.getListValue();
							callback(event.getTag(), event.getType(), l->inner(), l->length, event.getTime());
						} else if (event.getType() == RNBO::MessageEvent::Type::Number) {
							RNBO::number v = event.getNumValue();
							callback(event.getTag(), event.getType(), &v, 1, event.getTime());
						} else if (event.getType() == RNBO::MessageEvent::Type::Bang) {
							callback(event.getTag(), event.getType(), nullptr, 0, event.getTime());
						} else {
							//??
						}
					}
			});
	});
}
#endif
