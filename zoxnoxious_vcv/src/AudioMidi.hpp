#include "plugin.hpp"

/* this code is pretty much all taken from VCV Rack's Fundamental.  */

struct ZoxnoxiousMidiOutput : midi::Output {

    ZoxnoxiousMidiOutput() {
        reset();
    }


    void reset() {
        Output::reset();
        setChannel(-1); // allow messages out on any channel
    }

    void sendMidiMessage(midi::Message midiMessage) {
        Output::sendMessage(midiMessage);
    }

};



// this ought to be chopped in half since it only really
// uses the inputs.  Outputs could be removed.

// NOTE: THIS DOES NOT CLAMP: User of this audio port should clamp all inputs.
// Maybe I'll change that later... the unmeasurable effieciency gained
// is perhaps the wrong optimization.  The inputs may have varying ranges
// depending on the hardware and the particular signal, so it makes sense the
// hardware module writer implementor better know the clamping
// requirements for each signal.
const int num_audio_inputs = 31;
const int num_audio_outputs = 31;
struct ZoxnoxiousAudioPort : audio::Port {
	Module* module;

	dsp::DoubleRingBuffer<dsp::Frame<num_audio_inputs>, 32768> engineInputBuffer;
	dsp::DoubleRingBuffer<dsp::Frame<num_audio_outputs>, 32768> engineOutputBuffer;

	dsp::SampleRateConverter<num_audio_inputs> inputSrc;
	dsp::SampleRateConverter<num_audio_outputs> outputSrc;

	// Port variable caches
	int deviceNumInputs = 0;
	int deviceNumOutputs = 0;
	float deviceSampleRate = 0.f;
	int requestedEngineFrames = 0;

	ZoxnoxiousAudioPort(Module* module) {
		this->module = module;
		maxOutputs = num_audio_inputs;
		maxInputs = num_audio_outputs;
		inputSrc.setQuality(6);
		outputSrc.setQuality(6);
	}

	void setMaster(bool master = true) {
		if (master) {
			APP->engine->setMasterModule(module);
		}
		else {
			// Unset master only if module is currently master
			if (isMaster())
				APP->engine->setMasterModule(NULL);
		}
	}

	bool isMaster() {
		return APP->engine->getMasterModule() == module;
	}

	void processInput(const float* input, int inputStride, int frames) override {
		deviceNumInputs = std::min(getNumInputs(), num_audio_outputs);
		deviceNumOutputs = std::min(getNumOutputs(), num_audio_inputs);
		deviceSampleRate = getSampleRate();

		// DEBUG("%p: new device block ____________________________", this);
		// Claim master module if there is none
		if (!APP->engine->getMasterModule()) {
			setMaster();
		}
		bool isMasterCached = isMaster();

		// Set sample rate of engine if engine sample rate is "auto".
		if (isMasterCached) {
			APP->engine->setSuggestedSampleRate(deviceSampleRate);
		}

		float engineSampleRate = APP->engine->getSampleRate();
		float sampleRateRatio = engineSampleRate / deviceSampleRate;

		// Consider engine buffers "too full" if they contain a bit more than the audio device's number of frames, converted to engine sample rate.
		int maxEngineFrames = (int) std::ceil(frames * sampleRateRatio * 2.0) - 1;
		// If the engine output buffer is too full, clear it to keep latency low. No need to clear if master because it's always cleared below.
		if (!isMasterCached && (int) engineOutputBuffer.size() > maxEngineFrames) {
			engineOutputBuffer.clear();
			// DEBUG("%p: clearing engine output", this);
		}

		if (deviceNumInputs > 0) {
			// Always clear engine output if master
			if (isMasterCached) {
				engineOutputBuffer.clear();
			}
			// Set up sample rate converter
			outputSrc.setRates(deviceSampleRate, engineSampleRate);
			outputSrc.setChannels(deviceNumInputs);
			int inputFrames = frames;
			int outputFrames = engineOutputBuffer.capacity();
			outputSrc.process(input, inputStride, &inputFrames, (float*) engineOutputBuffer.endData(), num_audio_outputs, &outputFrames);
			engineOutputBuffer.endIncr(outputFrames);
			// Request exactly as many frames as we have in the engine output buffer.
			requestedEngineFrames = engineOutputBuffer.size();
		}
		else {
			// Upper bound on number of frames so that `audioOutputFrames >= frames` when processOutput() is called.
			requestedEngineFrames = std::max((int) std::ceil(frames * sampleRateRatio) - (int) engineInputBuffer.size(), 0);
		}
	}

	void processBuffer(const float* input, int inputStride, float* output, int outputStride, int frames) override {
		// Step engine
		if (isMaster() && requestedEngineFrames > 0) {
			// DEBUG("%p: %d block, stepping %d", this, frames, requestedEngineFrames);
			APP->engine->stepBlock(requestedEngineFrames);
		}
	}

	void processOutput(float* output, int outputStride, int frames) override {
		// bool isMasterCached = isMaster();
		float engineSampleRate = APP->engine->getSampleRate();
		float sampleRateRatio = engineSampleRate / deviceSampleRate;

		if (deviceNumOutputs > 0) {
			// Set up sample rate converter
			inputSrc.setRates(engineSampleRate, deviceSampleRate);
			inputSrc.setChannels(deviceNumOutputs);
			// Convert engine input -> audio output
			int inputFrames = engineInputBuffer.size();
			int outputFrames = frames;
			inputSrc.process((const float*) engineInputBuffer.startData(), num_audio_inputs, &inputFrames, output, outputStride, &outputFrames);
			engineInputBuffer.startIncr(inputFrames);
			// Clamp output samples
			for (int i = 0; i < outputFrames; i++) {
				for (int j = 0; j < deviceNumOutputs; j++) {
					float v = output[i * outputStride + j];
                                        // strictly speaking the clamp isn't necessary
                                        // if the user of this audioport does the
                                        // clamping (and it does below).
                                        // Uncomment to get belt & suspenders approach.
					//v = clamp(v, -1.f, 1.f);
					output[i * outputStride + j] = v;
				}
			}
			// Fill the rest of the audio output buffer with zeros
			for (int i = outputFrames; i < frames; i++) {
				for (int j = 0; j < deviceNumOutputs; j++) {
					output[i * outputStride + j] = 0.f;
				}
			}
		}


		// If the engine input buffer is too full, clear it to keep latency low.
		int maxEngineFrames = (int) std::ceil(frames * sampleRateRatio * 2.0) - 1;
		if ((int) engineInputBuffer.size() > maxEngineFrames) {
			engineInputBuffer.clear();
		}

	}

	void onStartStream() override {
		engineInputBuffer.clear();
		engineOutputBuffer.clear();
	}

	void onStopStream() override {
		deviceNumInputs = 0;
		deviceNumOutputs = 0;
		deviceSampleRate = 0.f;
		engineInputBuffer.clear();
		engineOutputBuffer.clear();
		// We can be in an Engine write-lock here (e.g. onReset() calls this indirectly), so use non-locking master module API.
		// setMaster(false);
		if (APP->engine->getMasterModule() == module) {
			APP->engine->setMasterModule_NoLock(NULL);
                }
	}
};

