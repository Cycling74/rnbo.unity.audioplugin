#include <AudioPluginUtil.h>
#include <RNBO.h>
#include <vector>
#include <mutex>
#include <limits>
#include <atomic>
#include <readerwriterqueue/readerwriterqueue.h>

#include <rnbo_description.h>
#include <iostream>


// if there is no shared lock, we simply use unique lock
// there may be a slight performance hit when calling functions that use
// with_instance from multiple threads but that might not actually happen anyway
#ifdef NO_SHARED_LOCK
using rw_mutex = std::mutex;
using write_lock = std::lock_guard<std::mutex>;
using read_lock = std::lock_guard<std::mutex>;
#else
#include <shared_mutex>
using rw_mutex = std::shared_mutex;
using write_lock = std::unique_lock<std::shared_mutex>;
using read_lock = std::shared_lock<std::shared_mutex>;
#endif

using RNBO::ParameterType;

//callbacks
extern "C" {
	typedef void (UNITY_AUDIODSP_CALLBACK * CParameterEventCallback)(void * handle, size_t, RNBO::ParameterValue, RNBO::MillisecondTime);
	typedef void (UNITY_AUDIODSP_CALLBACK * CMessageEventCallback)(void * handle, uint32_t, size_t, RNBO::ParameterValue *, size_t, RNBO::MillisecondTime);
	typedef void (UNITY_AUDIODSP_CALLBACK * CReleaseDataRefCallback)(void * handle, size_t);

	typedef void (UNITY_AUDIODSP_CALLBACK * CTransportEventCallback)(void * handle, bool, RNBO::MillisecondTime);
	typedef void (UNITY_AUDIODSP_CALLBACK * CTempoEventCallback)(void * handle, RNBO::number, RNBO::MillisecondTime);
	typedef void (UNITY_AUDIODSP_CALLBACK * CBeatTimeEventCallback)(void * handle, RNBO::number, RNBO::MillisecondTime);
	typedef void (UNITY_AUDIODSP_CALLBACK * CTimeSignatureEventCallback)(void * handle, int32_t, int32_t, RNBO::MillisecondTime);

	typedef void (UNITY_AUDIODSP_CALLBACK * CTransportRequestCallback)(void * handle, RNBO::MillisecondTime time, uint8_t* running, RNBO::number* bpm, RNBO::number* beatTime, int32_t *timeSigNum, int32_t *timeSigDenom);
	typedef void (UNITY_AUDIODSP_CALLBACK * CPresetCallback)(void * handle, const char * payload);
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
namespace {

	class Callback {
		public:
			Callback(void (* cb)(), void * handle) : mCallback(cb), mHandle(handle) {}
			template<typename T>
				T callback() const { return reinterpret_cast<T>(mCallback); }
			void * handle() const { return mHandle; }
		private:
			//static method that we don't have to release
			void (* mCallback)();
			//GCHandle that we need to release
			void * mHandle;
	};

	//we have a pointer to a GCHandle that we are holding, we need to notify the c# side that it should release
	std::mutex callbackReleaseQueueMutex; //only for reading from it
	moodycamel::ReaderWriterQueue<Callback *, 32> callbackReleaseQueue;
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
			typedef std::function<void(std::shared_ptr<const RNBO::Preset>)> PresetCallback;

			UnityEventHandler(
					MessageEventCallback mc = nullptr,
					MidiEventCallback mec = nullptr,

					TransportEventCallback transportCallback = nullptr,
					TempoEventCallback tempoCallback = nullptr,
					BeatTimeEventCallback beatTimeCallback = nullptr,
					TimeSignatureEventCallback timeSigCallback = nullptr,

					ParameterEventCallback pc = nullptr,
					PresetTouchedCallback pt = nullptr,
					PresetCallback psc = nullptr
					) :
				mEventsAvailable(false),
				mMessageEventCallback(mc),
				mMidiEventCallback(mec),

				mTransportEventCallback(transportCallback),
				mTempoEventCallback(tempoCallback),
				mBeatTimeEventCallback(beatTimeCallback),
				mTimeSignatureEventCallback(timeSigCallback),

				mParameterEventCallback(pc),
				mPresetTouchedCallback(pt),
				mPresetCallback(psc)
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
			void setPresetCallback(PresetCallback cb) { mPresetCallback = cb; };

			//only call from the poll thread
			void clearCallbacks() {
				setMessageEventCallback(nullptr);
				setMidiEventCallback(nullptr);
				setTransportEventCallback(nullptr);
				setTempoEventCallback(nullptr);
				setBeatTimeEventCallback(nullptr);
				setTimeSignatureEventCallback(nullptr);
				setParameterEventCallback(nullptr);
				setPresetTouchedCallback(nullptr);
				setPresetCallback(nullptr);
			}

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

			void handlePreset(std::shared_ptr<const RNBO::Preset> p) {
				if (mPresetCallback)
					mPresetCallback(p);
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
			PresetCallback mPresetCallback;
	};

	const int32_t invalidKey = 0;
	const int32_t maxKey = 16777216;

	static std::atomic<Callback *> globalTransportCallback = nullptr;
	Callback * globalTransportCallbackCurrent = nullptr;

	struct InnerData {
			UnityEventHandler mEventHandler;
			RNBO::CoreObject mCore;
			int32_t mInstanceKey = invalidKey;

			std::atomic<Callback *> mTransportCallback = nullptr;
			Callback * mTransportCallbackCurrent = nullptr;

			bool mTransportRunning = false;
			RNBO::number mTransportBPM = 0.0;
			RNBO::number mTransportBeatTime = -1.0;
			int32_t mTransportTimeSigNum = 0;
			int32_t mTransportTimeSigDenom = 0;

			InnerData() : mCore(&mEventHandler) {}
			~InnerData() {
				if (mTransportCallbackCurrent) {
					callbackReleaseQueue.try_enqueue(mTransportCallbackCurrent);
				}
				auto transport = mTransportCallback.load();
				if (transport) {
					callbackReleaseQueue.try_enqueue(transport);
				}
			}

			void updateTimeAndTransport(RNBO::MillisecondTime now) {
				mCore.setCurrentTime(now);

				//sync to transport
				//first, release any old transports
				Callback * transport = mTransportCallback.load();
				if (transport != mTransportCallbackCurrent) {
					if (mTransportCallbackCurrent != nullptr) {
						callbackReleaseQueue.try_enqueue(mTransportCallbackCurrent);
					}
					mTransportCallbackCurrent = transport;
				}

				Callback * globalTransport = globalTransportCallback.load();
				if (globalTransport != globalTransportCallbackCurrent) {
					if (globalTransportCallbackCurrent != nullptr) {
						callbackReleaseQueue.try_enqueue(globalTransportCallbackCurrent);
					}
					globalTransportCallbackCurrent = globalTransport;
				}

				if (transport == nullptr)
					transport = globalTransport;

				if (transport != nullptr) {
					RNBO::number bpm = 0.0, beatTime = 0.0;
					int32_t timeSigNum = 4, timeSigDenom = 4;

					uint8_t runningByte = 0;
					transport->callback<CTransportRequestCallback>()(
							transport->handle(),
							now, &runningByte, &bpm, &beatTime, &timeSigNum, &timeSigDenom);

					bool running = runningByte != 0;
					if (running != mTransportRunning) {
						mTransportRunning = running;

						RNBO::TransportEvent event(now, running ? RNBO::TransportState::RUNNING : RNBO::TransportState::STOPPED);
						mCore.scheduleEvent(event);
					}

					if (bpm != mTransportBPM) {
						mTransportBPM = bpm;

						RNBO::TempoEvent event(now, bpm);
						mCore.scheduleEvent(event);
					}

					if (beatTime != mTransportBeatTime) {
						mTransportBeatTime = beatTime;

						RNBO::BeatTimeEvent event(now, beatTime);
						mCore.scheduleEvent(event);
					}

					if (timeSigNum != mTransportTimeSigNum || timeSigDenom != mTransportTimeSigDenom) {
						mTransportTimeSigNum = timeSigNum;
						mTransportTimeSigDenom = timeSigDenom;

						RNBO::TimeSignatureEvent event(now, timeSigNum, timeSigDenom);
						mCore.scheduleEvent(event);
					}

				}
			}
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
	rw_mutex instances_mutex;
	std::unordered_map<int32_t, InnerData *> instances;
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
				write_lock wlock(instances_mutex);
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

		auto& inner = effectdata->inner;

		const RNBO::MillisecondTime stoms = 1000.0;
		RNBO::MillisecondTime now = stoms * (static_cast<RNBO::MillisecondTime>(state->currdsptick) / static_cast<RNBO::MillisecondTime>(state->samplerate));
		inner.updateTimeAndTransport(now);

		inner.mCore.process(inbuffer, inchannels, outbuffer, outchannels, length, nullptr, nullptr);

		return UNITY_AUDIODSP_OK;
	}

	UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK SetFloatParameterCallback(UnityAudioEffectState* state, int index, float value) {
		EffectData* effectdata = state->GetEffectData<EffectData>();

		//set index map for later retrieval
#if RNBO_UNITY_INSTANCE_ACCESS_HACK == 1
		if (index == 0) {
			write_lock wlock(instances_mutex);
			int32_t key = static_cast<int32_t>(value);
			//integer precision
			assert(key >= 0 && key <= 16777216);
			if (key != invalidKey) {
				//test for collision
				auto it = instances.find(key);
				if (it != instances.end()) {
					it->second->mInstanceKey = invalidKey;
				}
				instances[key] = &effectdata->inner;
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

	bool with_instance(int32_t key, std::function<void(RNBOUnity::InnerData *)> func) {
		read_lock rlock(RNBOUnity::instances_mutex);
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

extern "C" UNITY_AUDIODSP_EXPORT_API void * AUDIO_CALLING_CONVENTION RNBOInstanceCreate(int32_t* outkey)
{
	write_lock wlock(RNBOUnity::instances_mutex);

	//TODO, better key lookup
	int32_t key = -1;
	while (RNBOUnity::instances.count(key) != 0 && key < 0) {
		key -= 1;
	}

	//XXX ERROR
	if (key >= 0) {
		return nullptr;
	}

	RNBOUnity::InnerData * i = new RNBOUnity::InnerData();
	i->mInstanceKey = key;
	RNBOUnity::instances.insert({ key, i });
	*outkey = key;
	return i;
}

extern "C" UNITY_AUDIODSP_EXPORT_API void AUDIO_CALLING_CONVENTION RNBOInstanceDestroy(RNBOUnity::InnerData * inst)
{
	auto key = inst->mInstanceKey;

	write_lock wlock(RNBOUnity::instances_mutex);
	auto it = RNBOUnity::instances.find(key);
	if (it == RNBOUnity::instances.end()) {
		//ERROR
	} else {
		delete inst;
		RNBOUnity::instances.erase(key);
	}
}

extern "C" UNITY_AUDIODSP_EXPORT_API void AUDIO_CALLING_CONVENTION RNBOProcess(RNBOUnity::InnerData * inner, RNBO::MillisecondTime now, float * buffer, int32_t channels, int32_t nframes, int32_t samplerate)
{
	inner->mCore.prepareToProcess(samplerate, nframes);
	inner->updateTimeAndTransport(now);
	inner->mCore.process(buffer, channels, buffer, channels, nframes, nullptr, nullptr);
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOInstanceMapped(int32_t key)
{
	return with_instance(key, [](RNBOUnity::InnerData*) { /*do nothing*/ });
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
	return with_instance(key, [payload](RNBOUnity::InnerData* inner) {
			try {
				auto preset = RNBO::convertJSONToPreset(std::string(payload));
				inner->mCore.setPreset(std::move(preset));
			} catch (std::exception& e) {
				std::cerr << "error converting preset payload to RNBO preset " << e.what() << std::endl;
			}
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOGetPresetSync(int32_t key, char ** payload)
{
	if (!payload) {
		return false;
	}

	*payload = nullptr;

	return with_instance(key, [payload](RNBOUnity::InnerData * inner) {
			try {
				auto preset = inner->mCore.getPresetSync();
				std::string s = RNBO::convertPresetToJSON(*preset);
				*payload = new char[s.size() + 1];
				std::strcpy(*payload, s.c_str());
			} catch (std::exception& e) {
				std::cerr << "error capturing preset " << e.what() << std::endl;
			}
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOGetPreset(int32_t key)
{
	return with_instance(key, [](RNBOUnity::InnerData * inner) {
			inner->mCore.getPreset([inner](std::shared_ptr<const RNBO::Preset> p) {
					inner->mEventHandler.handlePreset(p);
			});
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API void AUDIO_CALLING_CONVENTION RNBOFreePreset(char * payload)
{
	if (payload) {
		delete [] payload;
	}
}


extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOPoll(int32_t key)
{
	//service the shared release queue
	{

#ifndef NO_SHARED_LOCK
		std::unique_lock<std::mutex> guard(datarefReleaseQueueMutex, std::try_to_lock);
		if (guard.owns_lock()) {
#else
		{
			std::lock_guard<std::mutex> guard(datarefReleaseQueueMutex);
#endif
			char * d = nullptr;
			while (datarefReleaseQueue.try_dequeue(d)) {
				if (d) {
					delete [] d;
				}
			}
		}
	}

	return with_instance(key, [](RNBOUnity::InnerData * inner) {
			inner->mEventHandler.poll();
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API void * AUDIO_CALLING_CONVENTION RNBOReleaseHandles()
{
	void * handle = nullptr;
#ifndef NO_SHARED_LOCK
	std::unique_lock<std::mutex> guard(callbackReleaseQueueMutex, std::try_to_lock);
	if (guard.owns_lock()) {
#else
	{
		std::lock_guard<std::mutex> guard(callbackReleaseQueueMutex);
#endif
		Callback * d = nullptr;
		if (callbackReleaseQueue.try_dequeue(d)) {
			handle = d->handle();
			delete d;
		}
	}
	return handle;
}

extern "C" UNITY_AUDIODSP_EXPORT_API RNBO::MessageTag AUDIO_CALLING_CONVENTION RNBOTag(const char * tagChar)
{
	return RNBO::TAG(tagChar);
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOResolveTag(int32_t key, RNBO::MessageTag tag, const char** tagChar)
{
	return with_instance(key, [tag, tagChar](RNBOUnity::InnerData * inner) {
			if (tagChar) {
				*tagChar = inner->mCore.resolveTag(tag);
			}
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSetParamValue(int32_t key, RNBO::ParameterIndex index, RNBO::ParameterValue value, RNBO::MillisecondTime attime)
{
	return with_instance(key, [index, value, attime](RNBOUnity::InnerData * inner) {
			inner->mCore.setParameterValue(index, value, attime);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOGetParamValue(int32_t key, RNBO::ParameterIndex index, RNBO::ParameterValue * valueOut)
{
	return with_instance(key, [index, valueOut](RNBOUnity::InnerData * inner) {
			if (valueOut != nullptr) {
				*valueOut = inner->mCore.getParameterValue(index);
			}
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSetParamValueNormalized(int32_t key, RNBO::ParameterIndex index, RNBO::ParameterValue value, RNBO::MillisecondTime attime)
{
	return with_instance(key, [index, value, attime](RNBOUnity::InnerData * inner) {
			inner->mCore.setParameterValueNormalized(index, value, attime);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOGetParamValueNormalized(int32_t key, RNBO::ParameterIndex index, RNBO::ParameterValue * valueOut)
{
	return with_instance(key, [index, valueOut](RNBOUnity::InnerData * inner) {
			if (valueOut != nullptr) {
				*valueOut = inner->mCore.convertToNormalizedParameterValue(index, inner->mCore.getParameterValue(index));
			}
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSendMessageNumber(int32_t key, RNBO::MessageTag tag, RNBO::number v, RNBO::MillisecondTime attime)
{
	return with_instance(key, [&tag, v, attime](RNBOUnity::InnerData * inner) {
			RNBO::MessageEvent event(tag, attime, v);
			inner->mCore.scheduleEvent(event);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSendMessageBang(int32_t key, RNBO::MessageTag tag, RNBO::MillisecondTime attime)
{
	return with_instance(key, [tag, attime](RNBOUnity::InnerData * inner) {
			RNBO::MessageEvent event(tag, attime);
			inner->mCore.scheduleEvent(event);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSendMessageList(int32_t key, RNBO::MessageTag tag, const RNBO::number* buffer, size_t bufferlen, RNBO::MillisecondTime attime)
{
	return with_instance(key, [tag, buffer, bufferlen, attime](RNBOUnity::InnerData * inner) {
		auto l = std::make_unique<RNBO::list>();
		for (auto i = 0; i < bufferlen; i++) {
			l->push(buffer[i]);
		}
		RNBO::MessageEvent event(tag, attime, std::move(l));
		inner->mCore.scheduleEvent(event);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSendMIDI(int32_t key, const uint8_t* bytes, int len, RNBO::MillisecondTime attime)
{
	return with_instance(key, [bytes, len, attime](RNBOUnity::InnerData * inner) {
			RNBO::MidiEvent event(attime, 0, bytes, len);
			inner->mCore.scheduleEvent(event);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSendTransportEvent(int32_t key, bool running, RNBO::MillisecondTime attime)
{
	return with_instance(key, [running, attime](RNBOUnity::InnerData * inner) {
			RNBO::TransportEvent event(attime, running ? RNBO::TransportState::RUNNING : RNBO::TransportState::STOPPED);
			inner->mCore.scheduleEvent(event);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSendTempoEvent(int32_t key, RNBO::number bpm, RNBO::MillisecondTime attime)
{
	return with_instance(key, [bpm, attime](RNBOUnity::InnerData * inner) {
			RNBO::TempoEvent event(attime, bpm);
			inner->mCore.scheduleEvent(event);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSendBeatTimeEvent(int32_t key, RNBO::number beattime, RNBO::MillisecondTime attime)
{
	return with_instance(key, [beattime, attime](RNBOUnity::InnerData * inner) {
			RNBO::BeatTimeEvent event(attime, beattime);
			inner->mCore.scheduleEvent(event);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOSendTimeSignatureEvent(int32_t key, int32_t numerator, int32_t denominator, RNBO::MillisecondTime attime)
{
	return with_instance(key, [numerator, denominator, attime](RNBOUnity::InnerData * inner) {
			RNBO::TimeSignatureEvent event(attime, numerator, denominator);
			inner->mCore.scheduleEvent(event);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOCopyLoadDataRef(int32_t key, const char * id, const float * data, size_t datalen, size_t channels, size_t samplerate)
{
	return with_instance(key, [id, data, datalen, channels, samplerate](RNBOUnity::InnerData * inner) {
			RNBO::Float32AudioBuffer bufferType(channels, static_cast<double>(samplerate));

			//we need to create our own copy as RNBO might write into the data AND it also might realloc
			float * d = new float[datalen];
			size_t bytes = sizeof(float) * datalen;
			std::memcpy(d, data, bytes);
			inner->mCore.setExternalData(id, reinterpret_cast<char *>(d), bytes, bufferType, DataRefRelease);
	});
}

// here we simply send over the pointer to the data, since RNBO buffers are read/write, you have to be sure that your code doesn't try to write or resize
// TODO can we indicate a real release?
extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOUnsafeLoadReadOnlyDataRef(int32_t key, const char * id, const float * data, size_t datalen, size_t channels, size_t samplerate)
{
	return with_instance(key, [id, data, datalen, channels, samplerate](RNBOUnity::InnerData * inner) {
			RNBO::Float32AudioBuffer bufferType(channels, static_cast<double>(samplerate));

			size_t bytes = sizeof(float) * datalen;
      char * ptr = const_cast<char *>(reinterpret_cast<const char *>(data));
			inner->mCore.setExternalData(id, ptr, bytes, bufferType, nullptr);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOReleaseDataRef(int32_t key, const char * id)
{
	return with_instance(key, [id](RNBOUnity::InnerData * inner) {
			inner->mCore.releaseExternalData(id);
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBOClearRegisteredCallbacks(int32_t key)
{
	return with_instance(key, [](RNBOUnity::InnerData * inner) {
			inner->mEventHandler.clearCallbacks();
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBORegisterParameterEventCallback(int32_t key, CParameterEventCallback callback, void * handle)
{
	return with_instance(key, [callback, handle](RNBOUnity::InnerData * inner) {
			inner->mEventHandler.setParameterEventCallback([callback, handle](RNBO::ParameterEvent event) {
					if (callback && handle) {
						callback(handle, event.getIndex(), event.getValue(), event.getTime());
					}
			});
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBORegisterMessageEventCallback(int32_t key, CMessageEventCallback callback, void * handle)
{
	return with_instance(key, [callback, handle](RNBOUnity::InnerData * inner) {
			inner->mEventHandler.setMessageEventCallback([callback, handle](RNBO::MessageEvent event) {
					if (callback && handle && event.getObjectId() == 0) {
						if (event.getType() == RNBO::MessageEvent::Type::List) {
							//hold shared pointer
							auto l = event.getListValue();
							callback(handle, event.getTag(), event.getType(), l->inner(), l->length, event.getTime());
						} else if (event.getType() == RNBO::MessageEvent::Type::Number) {
							RNBO::number v = event.getNumValue();
							callback(handle, event.getTag(), event.getType(), &v, 1, event.getTime());
						} else if (event.getType() == RNBO::MessageEvent::Type::Bang) {
							callback(handle, event.getTag(), event.getType(), nullptr, 0, event.getTime());
						} else {
							//??
						}
					}
			});
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBORegisterTransportEventCallback(int32_t key, CTransportEventCallback callback, void * handle)
{
	return with_instance(key, [callback, handle](RNBOUnity::InnerData * inner) {
			inner->mEventHandler.setTransportEventCallback([callback, handle](RNBO::TransportEvent event) {
					if (callback && handle) {
						callback(handle, event.getState() == RNBO::TransportState::RUNNING, event.getTime());
					}
			});
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBORegisterTempoEventCallback(int32_t key, CTempoEventCallback callback, void * handle)
{
	return with_instance(key, [callback, handle](RNBOUnity::InnerData * inner) {
			inner->mEventHandler.setTempoEventCallback([callback, handle](RNBO::TempoEvent event) {
					if (callback && handle) {
						callback(handle, event.getTempo(), event.getTime());
					}
			});
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBORegisterBeatTimeEventCallback(int32_t key, CBeatTimeEventCallback callback, void * handle)
{
	return with_instance(key, [callback, handle](RNBOUnity::InnerData * inner) {
			inner->mEventHandler.setBeatTimeEventCallback([callback, handle](RNBO::BeatTimeEvent event) {
					if (callback && handle) {
						callback(handle, event.getBeatTime(), event.getTime());
					}
			});
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBORegisterTimeSignatureEventCallback(int32_t key, CTimeSignatureEventCallback callback, void * handle)
{
	return with_instance(key, [callback, handle](RNBOUnity::InnerData * inner) {
			inner->mEventHandler.setTimeSignatureEventCallback([callback, handle](RNBO::TimeSignatureEvent event) {
					if (callback && handle) {
						callback(handle, static_cast<int32_t>(event.getNumerator()), static_cast<int32_t>(event.getDenominator()), event.getTime());
					}
			});
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API void AUDIO_CALLING_CONVENTION RNBORegisterGlobalTransportRequestCallback(CTransportRequestCallback callback, void * handle)
{
	if (callback != nullptr && handle != nullptr) {
		RNBOUnity::globalTransportCallback.store(new Callback(reinterpret_cast<void (*)()>(callback), handle));
	} else {
		RNBOUnity::globalTransportCallback.store(nullptr);
	}
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBORegisterTransportRequestCallback(int32_t key, CTransportRequestCallback callback, void * handle)
{
	return with_instance(key, [callback, handle](RNBOUnity::InnerData * inner) {
			if (callback != nullptr && handle != nullptr) {
			Callback * data = new Callback(reinterpret_cast<void (*)()>(callback), handle);
			inner->mTransportCallback.store(data);
			} else {
			inner->mTransportCallback.store(nullptr);
			}
	});
}

extern "C" UNITY_AUDIODSP_EXPORT_API bool AUDIO_CALLING_CONVENTION RNBORegisterPresetCallback(int32_t key, CPresetCallback callback, void * handle)
{
	return with_instance(key, [callback, handle](RNBOUnity::InnerData * inner) {
			inner->mEventHandler.setPresetCallback([callback, handle](std::shared_ptr<const RNBO::Preset> preset) {
					if (callback && handle) {
						std::string s = RNBO::convertPresetToJSON(*preset);
						callback(handle, s.c_str());
					}
			});
	});
}

#endif
