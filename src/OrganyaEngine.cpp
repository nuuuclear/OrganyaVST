#include "OrganyaEngine.h"
#include "BinaryData.h"

#include <cmath>

namespace {
constexpr size_t fullWaveBankSize = 100u * 256u;
constexpr float attackSeconds = 0.004f;
constexpr float releaseSeconds = 0.12f;
}

OrganyaEngine::OrganyaEngine() {
    waveBankReady = loadWaveBank();
    loadDrumSamples();
    reset();
}

// TODO: better handling of drum samples
bool OrganyaEngine::loadDrumSamples() {
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    // should be the original samples.
    static constexpr const char* names[] = {
        "Bass04_wav", "Snare03_wav", "HiClose2_wav", "HiOpen2_wav",
        "Tom02_wav"
    };

    constexpr size_t drumSampleCount = sizeof(names) / sizeof(names[0]);

    bool allLoaded = true;
    for (size_t index = 0; index < drumSampleCount; ++index) {
        int size = 0;
        const auto* bytes = ResourceData::getNamedResource(names[index], size);
        if (bytes == nullptr || size <= 0) {
            allLoaded = false;
            continue;
        }

        auto input = std::make_unique<juce::MemoryInputStream>(bytes, static_cast<size_t>(size), false);
        std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(std::move(input)));
        if (reader == nullptr || reader->lengthInSamples <= 0) {
            allLoaded = false;
            continue;
        }

        drumSourceRate = reader->sampleRate;
        drumSamples[index].resize(static_cast<size_t>(reader->lengthInSamples));
        juce::AudioBuffer<float> decoded(1, static_cast<int>(reader->lengthInSamples));
        reader->read(&decoded, 0, decoded.getNumSamples(), 0, true, false);
        std::copy(
            decoded.getReadPointer(0), 
            decoded.getReadPointer(0) + decoded.getNumSamples(), 
            drumSamples[index].begin()
        );
    }

    return allLoaded;
}

bool OrganyaEngine::loadWaveBank() {
    if (ResourceData::WAVE100_wdbSize <= 0) return false;

    const auto* source = reinterpret_cast<const int8_t*>(ResourceData::WAVE100_wdb);
    const auto sourceSize = static_cast<size_t>(ResourceData::WAVE100_wdbSize);
    const auto hasAllWaves = sourceSize >= fullWaveBankSize;
    const auto sourceWaveSize = hasAllWaves ? waveSize : sourceSize;

    if (!hasAllWaves && sourceWaveSize == 0) return false;

    for (int wave = 0; wave < waveCount; ++wave) {
        for (int sample = 0; sample < waveSize; ++sample) {
            const auto waveOffset 
                = hasAllWaves 
                ? static_cast<size_t>(wave * waveSize) 
                : 0u;
            const auto sourcePosition = static_cast<size_t>(
                (static_cast<double>(sample) / waveSize) * sourceWaveSize
            ) % sourceWaveSize;

            waves[wave][sample] = source[waveOffset + sourcePosition] / 128.0f;
        }
    }

    return true;
}

void OrganyaEngine::reset() {
    for (auto& voice : voices) 
        voice = {};
    for (auto& voice : drumVoices)
        voice = {};
}

void OrganyaEngine::setSampleRate(double newSampleRate) {
    sampleRate = juce::jmax(1.0, newSampleRate);
}

void OrganyaEngine::setSampleIndex(int sampleIndex) {
    selectedSample = juce::jlimit(0, waveCount - 1, sampleIndex);
}

void OrganyaEngine::setPolyphony(int newPolyphony) {
    polyphony = juce::jlimit(1, 8, newPolyphony);
}

void OrganyaEngine::setDrumMode(bool enabled) {
    setMode(enabled ? 1 : 0);
}

void OrganyaEngine::setMode(int newMode) {
    newMode = juce::jlimit(0, 2, newMode);
    if (mode != newMode)
        reset();
    mode = newMode;
    drumMode = mode != 0;
}

void OrganyaEngine::setDrumSample(int sample) {
    selectedDrumSample = juce::jlimit(0, maxDrumVoices, sample);
}

void OrganyaEngine::noteOn(int channel, int note, float velocity) {
    if (auto* existing = findVoice(channel, note)) {
        existing->phase = 0.0;
        existing->velocity = velocity;
        existing->level = 0.0f;
        existing->released = false;

        return;
    }

    auto activeVoices = std::count_if(
        voices.begin(), voices.end(), [](const Voice& item) {
            return item.note >= 0;
        }
    );
    auto voice = std::find_if(
        voices.begin(), 
        voices.end(), 
        [](const Voice& item) { 
            return item.note < 0; 
        }
    );

    if (activeVoices >= polyphony) {
        voice = voices.end();
        for (auto candidate = voices.begin(); candidate != voices.end(); ++candidate) {
            if (candidate->note >= 0
                && (voice == voices.end() || candidate->level < voice->level)) {
                voice = candidate;
            }
        }
    } else if (voice == voices.end()) {
        voice = std::min_element(
            voices.begin(), 
            voices.end(), 
            [](const Voice& a, const Voice& b) { 
                return a.level < b.level; 
            }
        );
    }

    voice->note = note;
    voice->channel = channel;
    voice->wave = selectedSample;
    voice->velocity = velocity;
    voice->level = 0.0f;
    voice->pan = 0.5f;
    voice->phase = 0.0;
    voice->released = false;
}

void OrganyaEngine::noteOff(int channel, int note) {
    if (auto* voice = findVoice(channel, note)) {
        voice->released = true;
    }
}

void OrganyaEngine::drumNoteOn(int note, float velocity) {
    static constexpr int drumNotes[] = { 36, 38, 42, 46, 45, 39, 49, 51 };
    auto type = 0;
    auto distance = std::abs(note - drumNotes[0]);
    for (int index = 1; index < 8; ++index) {
        const auto candidateDistance = std::abs(note - drumNotes[index]);
        if (candidateDistance < distance) {
            distance = candidateDistance;
            type = index;
        }
    }

    auto voice = std::find_if(drumVoices.begin(), drumVoices.end(), [](const DrumVoice& item) {
        return item.note < 0;
    });

    if (voice == drumVoices.end()) {
            voice = std::min_element(
                drumVoices.begin(), drumVoices.end(), 
            [](const DrumVoice& a, const DrumVoice& b) {
                return a.level < b.level;
            }
        );
    }

    voice->note = note;
    voice->type = selectedDrumSample == 0 ? type : selectedDrumSample - 1;
    voice->level = 1.0f;
    voice->velocity = velocity;
    voice->pan = type == 2 ? 0.62f : (type == 6 ? 0.38f : 0.5f);
    voice->phase = 0.0;
    voice->samplePosition = 0.0;
}

OrganyaEngine::Voice* OrganyaEngine::findVoice(int channel, int note) {
    auto voice = std::find_if(
        voices.begin(), voices.end(), 
        [channel, note](const Voice& item
    ) {
        return item.channel == channel && item.note == note;
    });

    return voice == voices.end() ? nullptr : &*voice;
}

float OrganyaEngine::interpolateWave(const std::array<float, waveSize>& wave, double phase) {
    const auto position = phase * waveSize;
    const auto index = static_cast<int>(position) & (waveSize - 1);
    const auto next = (index + 1) & (waveSize - 1);

    return juce::jmap(
        static_cast<float>(position - std::floor(position)),
        wave[index],
        wave[next]
    );
}

void OrganyaEngine::renderVoices(
    juce::AudioBuffer<float>& buffer, 
    int start, 
    int count, 
    juce::SmoothedValue<float>& masterGain
) {
    const auto attackStep = 1.0f / juce::jmax(1.0f, 
        attackSeconds * static_cast<float>(sampleRate)
    );
    const auto releaseStep = 1.0f / juce::jmax(1.0f, 
        releaseSeconds * static_cast<float>(sampleRate)
    );

    for (auto& voice : voices) {
        if (voice.note < 0) continue;

        const auto frequency = 440.0 * std::pow(2.0, (voice.note - 69) / 12.0);
        const auto increment = frequency / sampleRate;
        const auto& wave = waves[voice.wave % waveCount];

        for (int offset = 0; offset < count; ++offset) {
            voice.level = voice.released
                ? voice.level - releaseStep
                : juce::jmin(1.0f, voice.level + attackStep);

            if (voice.level <= 0.0f) {
                voice = {};
                break;
            }

            const auto value 
                = interpolateWave(wave, voice.phase) 
                * voice.level 
                * voice.velocity 
                * masterGain.getNextValue()
                * 0.18f;
            
            if (buffer.getNumChannels() > 0)
                buffer.addSample(0, start + offset, value);
            if (buffer.getNumChannels() > 1)
                buffer.addSample(1, start + offset, value);

            voice.phase = std::fmod(voice.phase + increment, 1.0);
        }
    }
}

void OrganyaEngine::renderDrums(
    juce::AudioBuffer<float>& buffer, 
    int start, 
    int count, 
    juce::SmoothedValue<float>& masterGain
) {
    for (auto& voice : drumVoices) {
        if (voice.note < 0)
            continue;

        const auto& source = drumSamples[static_cast<size_t>(voice.type)];
        if (source.empty()) {
            voice = {};
            continue;
        }

        for (int offset = 0; offset < count; ++offset) {
            const auto sourceIndex = static_cast<size_t>(voice.samplePosition);
            if (sourceIndex >= source.size()) {
                voice = {};
                break;
            }

            const auto fraction = static_cast<float>(voice.samplePosition - sourceIndex);
            const auto nextIndex = juce::jmin(sourceIndex + 1, source.size() - 1);
            const auto sample = juce::jmap(fraction, source[sourceIndex], source[nextIndex]);

            const auto value = sample * voice.velocity * masterGain.getNextValue() * 0.22f;
            if (buffer.getNumChannels() > 0)
                buffer.addSample(0, start + offset, value * (1.0f - voice.pan));
            if (buffer.getNumChannels() > 1)
                buffer.addSample(1, start + offset, value * voice.pan);
            voice.samplePosition += drumSourceRate / sampleRate;
        }
    }
}

void OrganyaEngine::render(
    juce::AudioBuffer<float>& buffer,
    const juce::MidiBuffer& midi,
    juce::SmoothedValue<float>& masterGain
) {
    if ((!waveBankReady && !drumMode) || buffer.getNumSamples() <= 0) {
        return;
    }

    auto blockStart = 0;

    for (const auto metadata : midi) {
        const auto samplePosition = juce::jlimit(
            blockStart, buffer.getNumSamples(), metadata.samplePosition
        );

        const auto count = samplePosition - blockStart;
        if (count > 0 && !drumMode) {
            renderVoices(buffer, blockStart, count, masterGain);
        }

        if (count > 0 && drumMode) {
            renderDrums(buffer, blockStart, count, masterGain);
        }

        const auto message = metadata.getMessage();
        if (message.isNoteOn()) {
            if (drumMode) {
                drumNoteOn(message.getNoteNumber(), message.getFloatVelocity());
            } else {
                noteOn(message.getChannel(), 
                message.getNoteNumber(), 
                message.getFloatVelocity());
            }

        } else if (message.isNoteOff()) {
            if (!drumMode)
                noteOff(message.getChannel(), message.getNoteNumber());

        } else if (message.isAllNotesOff() || message.isAllSoundOff()) {
            reset();
        }

        blockStart = samplePosition;
    }

    const auto remaining = buffer.getNumSamples() - blockStart;

    if (drumMode) {
        renderDrums(buffer, blockStart, remaining, masterGain);
    } else {
        renderVoices(buffer, blockStart, remaining, masterGain);
    }
}
