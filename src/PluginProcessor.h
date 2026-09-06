#pragma once

#include <JuceHeader.h>
#include "OrganyaEngine.h"

class OrganyaAP final : public juce::AudioProcessor {
public:
	OrganyaAP();
	~OrganyaAP() override = default;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override { return true; }

	const juce::String getName() const override { return "Organya"; }
	bool acceptsMidi() const override { return true; }
	bool producesMidi() const override { return false; }
	bool isMidiEffect() const override { return false; }
	double getTailLengthSeconds() const override { return 0.0; }
	int getNumPrograms() override { return 1; }
	int getCurrentProgram() override { return 0; }
	void setCurrentProgram(int) override {}
	const juce::String getProgramName(int) override { return {}; }
	void changeProgramName(int, const juce::String&) override {}

	void getStateInformation(juce::MemoryBlock&) override;
	void setStateInformation(const void*, int) override;

	juce::AudioProcessorValueTreeState parameters;

private:
	juce::SmoothedValue<float> masterGain;
	OrganyaEngine organya;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrganyaAP)
};
