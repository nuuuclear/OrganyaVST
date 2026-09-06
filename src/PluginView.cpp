#include "PluginView.h"
#include "BinaryData.h"

namespace {
juce::Font createControlFont() {
    return juce::Font(juce::FontOptions().withName("Arial").withHeight(12.0f));
}

juce::Image loadBitmapFont() {
    juce::MemoryInputStream stream(
        ResourceData::font_png,
        static_cast<size_t>(ResourceData::font_pngSize),
        false
    );

    auto source = juce::ImageFileFormat::loadFrom(stream);
    if (!source.isValid()) return {};

    juce::Image mask(juce::Image::ARGB, source.getWidth(), source.getHeight(), true);

    juce::Image::BitmapData sourcePixels(
        source, 0, 0, 
        source.getWidth(), source.getHeight(), 
        juce::Image::BitmapData::readOnly
    );

    juce::Image::BitmapData maskPixels(
        mask, 0, 0, 
        mask.getWidth(), mask.getHeight(), 
        juce::Image::BitmapData::writeOnly
    );

    for (int y = 0; y < source.getHeight(); ++y) {
        for (int x = 0; x < source.getWidth(); ++x) {
            const auto pixel = sourcePixels.getPixelColour(x, y);
            const auto alpha = static_cast<juce::uint8>(
                pixel.getBrightness() > 0.5f ? 255 : 0
            );
            maskPixels.setPixelColour(
                x, y, 
                juce::Colour(juce::Colours::white.withAlpha(alpha / 255.0f))
            );
        }
    }

    return mask;
}
}

OrganyaAPEditor::OrganyaAPEditor(OrganyaAP& owner)
    : AudioProcessorEditor(&owner)
    , processor(owner)
    , bitmapFont(loadBitmapFont())
    , backgroundImage(juce::ImageCache::getFromMemory(
        ResourceData::organyavst_back_png,
        ResourceData::organyavst_back_pngSize)
    )
{
    setSize(460, 282);
    const auto font = createControlFont();
    resourceLookAndFeel.setDefaultSansSerifTypeface(font.getTypefacePtr());
    setLookAndFeel(&resourceLookAndFeel);

    title.setVisible(false);
    addAndMakeVisible(title);

    for (auto* label : { &masterLabel, &sampleLabel, &polyphonyLabel }) {
        label->setVisible(false);
        addAndMakeVisible(label);
    }

    setupSlider(masterSlider, "MASTER");
    setupNumericSlider(sampleSlider, "SAMPLE", 99.0);
    setupNumericSlider(polyphonySlider, "POLYPHONY", 8.0);
    modeSelector.addItemList({ "Waveform", "Organya Drums"}, 1);
    drumSampleSelector.addItemList({ "GM", "Kick", "Snare", "Closed Hat", "Open Hat", "Tom" }, 1);
    addAndMakeVisible(modeSelector);
    addAndMakeVisible(drumSampleSelector);
    modeSelector.onChange = [this] { updateModeVisibility(); };

#define AudioPVTS juce::AudioProcessorValueTreeState

    masterAttachment = std::make_unique<AudioPVTS::SliderAttachment>(
        processor.parameters, "master", masterSlider
    );

    sampleAttachment = std::make_unique<AudioPVTS::SliderAttachment>(
        processor.parameters, "sample", sampleSlider
    );

    polyphonyAttachment = std::make_unique<AudioPVTS::SliderAttachment>(
        processor.parameters, "polyphony", polyphonySlider
    );

    modeAttachment = std::make_unique<AudioPVTS::ComboBoxAttachment>(
        processor.parameters, "mode", modeSelector
    );

    drumSampleAttachment = std::make_unique<AudioPVTS::ComboBoxAttachment>(
        processor.parameters, "drumSample", drumSampleSelector
    );

#undef AudioPVTS

    updateModeVisibility();
}

OrganyaAPEditor::~OrganyaAPEditor() {
    setLookAndFeel(nullptr);
}

void OrganyaAPEditor::setupSlider(juce::Slider& slider, const juce::String& name) {

    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);

    slider.setRange(0.0, 1.0, 0.001);
    slider.setName(name);

    slider.setColour(juce::Slider::rotarySliderFillColourId,  juce::Colour(0xff6f9cd6));
    slider.setColour(juce::Slider::thumbColourId,             juce::Colour(0xfff7f2df));
    slider.setColour(juce::Slider::textBoxTextColourId,       juce::Colour(0xfff7f2df));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff242b2c));

    addAndMakeVisible(slider);
}

void OrganyaAPEditor::setupNumericSlider(
    juce::Slider& slider, 
    const juce::String& 
    name, 
    double maximum
) {
    slider.setSliderStyle(juce::Slider::IncDecButtons);
    slider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 70, 28);

    slider.setRange(name == "POLYPHONY" ? 1.0 : 0.0, maximum, 1.0);
    slider.setIncDecButtonsMode(juce::Slider::incDecButtonsNotDraggable);
    slider.setName(name);

    slider.setColour(juce::Slider::backgroundColourId,        juce::Colour(0xff242b2c));
    slider.setColour(juce::Slider::textBoxTextColourId,       juce::Colour(0xfff7f2df));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff242b2c));
    slider.setColour(juce::Slider::thumbColourId,             juce::Colour(0xff6f9cd6));

    addAndMakeVisible(slider);
}

void OrganyaAPEditor::updateModeVisibility() {
    const auto drum = modeSelector.getSelectedId() > 1;
    sampleSlider.setVisible(!drum);
    polyphonySlider.setVisible(!drum);
    drumSampleSelector.setVisible(drum);
    masterSlider.setVisible(true);
    repaint();
}

void OrganyaAPEditor::drawBitmapText(
    juce::Graphics& graphics, 
    const juce::String& text, 
    juce::Rectangle<int> area, 
    juce::Colour colour
) {
    constexpr int cellWidth = 8;
    constexpr int cellHeight = 12;
    constexpr int atlasX = 0;
    constexpr int atlasY = 0;

    const auto width = text.length() * cellWidth;
    auto x = area.getX() + (area.getWidth() - width) / 2;
    const auto y = area.getY() + (area.getHeight() - cellHeight) / 2;

    graphics.setColour(colour);
    graphics.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);

    for (auto character : text) {
        const auto code = static_cast<int>(character);

        if (code >= 32 && code <= 127) {
            const auto glyphIndex = code - 32;
            const auto row = glyphIndex / 32;
            const auto column = glyphIndex % 32;
            graphics.drawImage(bitmapFont, x, y, cellWidth, cellHeight,
                atlasX + column * cellWidth, atlasY + row * cellHeight,
                cellWidth, cellHeight, true);
        }

        x += cellWidth;
    }
}

void OrganyaAPEditor::paint(juce::Graphics& graphics) {
    // graphics.fillAll(juce::Colour(0xff000000));

    if (backgroundImage.isValid()) {
        graphics.drawImageAt(
            backgroundImage,
            0, //getLocalBounds().toFloat(),
            0 //juce::RectanglePlacement::stretchToFit
        );
    }

    // drawBitmapText(graphics, "Organya VST", { 24,  22,  300, 34 },  juce::Colour(0xfff7f2df));
    drawBitmapText(graphics, "Master",      { 170, 99,  120, 20 },  juce::Colour(0xffffffff));
    drawBitmapText(graphics, "Sample",      { 45,  115, 105, 20 },  juce::Colour(0xffffffff));
    drawBitmapText(graphics, "Polyphony",   { 305, 115, 105, 20 },  juce::Colour(0xffffffff));
}

void OrganyaAPEditor::resized() {
    title.setBounds(24, 22, 300, 34);

    masterSlider.setBounds(170, 115, 120, 130);
    sampleSlider.setBounds(45, 145, 105, 32);
    polyphonySlider.setBounds(305, 145, 105, 32);
    modeSelector.setBounds(170, 255, 120, 24);
    drumSampleSelector.setBounds(45, 145, 105, 32);

    masterLabel.setBounds(170, 99, 120, 20);
    sampleLabel.setBounds(45, 115, 105, 20);
    polyphonyLabel.setBounds(305, 115, 105, 20);
}
