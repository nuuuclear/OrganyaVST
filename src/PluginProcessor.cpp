#include "PluginProcessor.h"
#include "PluginView.h"

namespace {
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
            "master", "Master", juce::NormalisableRange<float>(0.0f, 1.0f), 0.72f)
        );

        parameters.push_back(std::make_unique<juce::AudioParameterInt>(
            "sample", "Sample", 0, 99, 0)
        );

        parameters.push_back(std::make_unique<juce::AudioParameterInt>(
            "polyphony", "Polyphony", 1, 8, 8)
        );

        parameters.push_back(std::make_unique<juce::AudioParameterChoice>(
            "mode", "Mode", juce::StringArray { "Waveform", "Organya Drums" }, 0)
        );

        parameters.push_back(std::make_unique<juce::AudioParameterChoice>(
            "drumSample", "Drum Sample", juce::StringArray {
                "GM", "Kick", "Snare", "Closed Hat", "Open Hat", "Tom"
            }, 0)
        );

        return {parameters.begin(), parameters.end()};
    }
}

OrganyaAP::OrganyaAP()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "PARAMETERS", createParameterLayout()) 
{
    masterGain.setCurrentAndTargetValue(parameters.getParameter("master")->getValue());
}

void OrganyaAP::prepareToPlay(double sampleRate, int) {
    organya.setSampleRate(sampleRate);

    masterGain.reset(sampleRate, 0.03);
    masterGain.setCurrentAndTargetValue(parameters.getParameter("master")->getValue());

}

void OrganyaAP::releaseResources() {}

bool OrganyaAP::isBusesLayoutSupported(const BusesLayout &layouts) const {
    const auto output = layouts.getMainOutputChannelSet();

    return output == juce::AudioChannelSet::mono() 
        || output == juce::AudioChannelSet::stereo();
}

void OrganyaAP::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi) {
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    masterGain.setTargetValue(parameters.getParameter("master")->getValue());
    const auto sampleParameter = parameters.getParameter("sample");

    organya.setSampleIndex(static_cast<int>(
        sampleParameter->convertFrom0to1(sampleParameter->getValue())
    ));

    const auto* polyphonyParameter = dynamic_cast<const juce::AudioParameterInt*>(
        parameters.getParameter("polyphony")
    );

    if (polyphonyParameter != nullptr)
        organya.setPolyphony(polyphonyParameter->get());

    const auto* modeParameter = parameters.getParameter("mode");
    const auto* drumSampleParameter = parameters.getParameter("drumSample");

    organya.setMode(static_cast<int>(
        modeParameter->convertFrom0to1(modeParameter->getValue())
    ));
    organya.setDrumSample(static_cast<int>(
        drumSampleParameter->convertFrom0to1(drumSampleParameter->getValue())
    ));

    organya.render(buffer, midi, masterGain);
}

void OrganyaAP::getStateInformation(juce::MemoryBlock &destination) {
    if (auto state = parameters.copyState(); auto xml = state.createXml()) {
        copyXmlToBinary(*xml, destination);
    }
}

void OrganyaAP::setStateInformation(const void *data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        if (xml->hasTagName(parameters.state.getType())) {
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
        }
    }
}

juce::AudioProcessorEditor *OrganyaAP::createEditor() {
    return new OrganyaAPEditor(*this);
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new OrganyaAP();
}
