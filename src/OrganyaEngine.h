#pragma once

#include <JuceHeader.h>

class OrganyaEngine {
public:
    OrganyaEngine();

    void reset();
    void setSampleRate(double sampleRate);
    void setSampleIndex(int sampleIndex);
    void setPolyphony(int polyphony);
    void setDrumMode(bool enabled);
    void setMode(int mode);
    void setDrumSample(int sample);
    void render(
        juce::AudioBuffer<float>& buffer, 
        const juce::MidiBuffer& midi, 
        juce::SmoothedValue<float>& masterGain
    );

private:
    struct Voice {
        int note = -1;
        int channel = 1;
        int wave = 0;
        bool released = false;
        float velocity = 0.0f;
        float level = 0.0f;
        float pan = 0.5f;
        double phase = 0.0;
    };

    struct DrumVoice {
        int note = -1;
        int type = 0;
        float level = 0.0f;
        float velocity = 0.0f;
        float pan = 0.5f;
        double phase = 0.0;
        double samplePosition = 0.0;
    };

    static constexpr int maxVoices = 32;
    static constexpr int maxDrumVoices = 8;
    static constexpr int waveCount = 100;
    static constexpr int waveSize = 256;

    std::array<Voice, maxVoices> voices;
    std::array<DrumVoice, maxDrumVoices> drumVoices;
    std::array<std::vector<float>, maxDrumVoices> drumSamples;
    double drumSourceRate = 44100.0;
    std::array<std::array<float, waveSize>, waveCount> waves {};
    bool waveBankReady = false;
    int selectedSample = 0;
    int polyphony = 8;
    bool drumMode = false;
    int mode = 0;
    int selectedDrumSample = 0;
    double sampleRate = 44100.0;

    bool loadWaveBank();
    bool loadDrumSamples();
    void noteOn(int channel, int note, float velocity);
    void noteOff(int channel, int note);
    void drumNoteOn(int note, float velocity);

    void renderVoices(
        juce::AudioBuffer<float>& buffer, 
        int start, 
        int count, 
        juce::SmoothedValue<float>& masterGain
    );

    void renderDrums(
        juce::AudioBuffer<float>& buffer, 
        int start,
        int count, 
        juce::SmoothedValue<float>& masterGain
    );

    Voice* findVoice(int channel, int note);
    static float interpolateWave(
        const std::array<float, waveSize>& wave, 
        double phase
    );
};