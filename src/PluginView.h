#pragma once

#include "PluginProcessor.h"

class PolyphonySlider : public juce::Slider {
public:
    PolyphonySlider() {
        setSliderStyle(juce::Slider::IncDecButtons);
        setTextBoxStyle(juce::Slider::TextBoxLeft, false, 70, 28);
    }

    juce::String getTextFromValue(double value) override {
        if (juce::approximatelyEqual(value, 1.0))
            return "MONO";

        return juce::String(static_cast<int>(value));
    }

    double getValueFromText(const juce::String& text) override {
        const auto trimmed = text.trim();

        if (trimmed.equalsIgnoreCase("MONO"))
            return 1.0;

        return trimmed.getDoubleValue();
    }
};

class OrganyaAPEditor final : public juce::AudioProcessorEditor {
public:
	explicit OrganyaAPEditor(OrganyaAP&);
	~OrganyaAPEditor() override;

	void paint(juce::Graphics&) override;
	void resized() override;

private:
	OrganyaAP& processor;
	juce::LookAndFeel_V4 resourceLookAndFeel;
	juce::Image bitmapFont;

	juce::Image backgroundImage;

	juce::Label title;
	juce::Label masterLabel;
	juce::Label sampleLabel;
	juce::Label polyphonyLabel;

	juce::Slider masterSlider;
	juce::Slider sampleSlider;
	PolyphonySlider polyphonySlider;
	juce::ToggleButton drumModeButton;
	juce::ComboBox modeSelector;
	juce::ComboBox drumSampleSelector;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterAttachment;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sampleAttachment;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> polyphonyAttachment;
	std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
	std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> drumSampleAttachment;
	void updateModeVisibility();

	void setupSlider(juce::Slider&, const juce::String&);
	void setupNumericSlider(juce::Slider&, const juce::String&, double maximum);
	void drawBitmapText(juce::Graphics&, const juce::String&, juce::Rectangle<int>, juce::Colour);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrganyaAPEditor)
};
