#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
class LaPautaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LaPautaLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a1114));
        setColour (juce::ComboBox::textColourId,       juce::Colour (0xfff2c27a));
        setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xffe8a24a));
        setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xffe8a24a));
        setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (0xff2a1114));
        setColour (juce::PopupMenu::textColourId,                  juce::Colour (0xfff2c27a));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff8a2f2a));
        setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);
        setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff2a1114));
        setColour (juce::TextButton::textColourOffId,  juce::Colour (0xfff2c27a));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
        const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
        auto area = bounds.withSizeKeepingCentre (diameter, diameter);
        const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);

        const float ringW = juce::jmax (3.0f, diameter * 0.09f);
        auto ringArea = area.reduced (ringW * 0.5f);
        const float radius = ringArea.getWidth() * 0.5f;
        const auto c = ringArea.getCentre();
        const float angle = startAngle + sliderPos * (endAngle - startAngle);

        juce::Path track;
        track.addCentredArc (c.x, c.y, radius, radius, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colour (0x55000000));
        g.strokePath (track, juce::PathStrokeType (ringW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path val;
        val.addCentredArc (c.x, c.y, radius, radius, 0.0f, startAngle, angle, true);
        g.setColour (accent);
        g.strokePath (val, juce::PathStrokeType (ringW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        auto body = area.reduced (ringW * 1.9f);
        g.setGradientFill (juce::ColourGradient (accent.brighter (0.6f), body.getX(), body.getY(),
                                                 accent.darker (0.7f), body.getRight(), body.getBottom(), false));
        g.fillEllipse (body);
        g.setColour (juce::Colour (0x66000000));
        g.drawEllipse (body, 1.2f);

        const auto bc = body.getCentre();
        const float r = body.getWidth() * 0.5f;
        const float sa = std::sin (angle), ca = -std::cos (angle);
        juce::Line<float> pointer (bc.x + sa * r * 0.35f, bc.y + ca * r * 0.35f,
                                   bc.x + sa * r * 0.92f, bc.y + ca * r * 0.92f);
        g.setColour (juce::Colour (0xff1c0a0c));
        g.drawLine (pointer, juce::jmax (2.0f, diameter * 0.04f));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (15.0f, juce::Font::bold)); }
};

//==============================================================================
class KnobCell : public juce::Component
{
public:
    KnobCell (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
              const juce::String& title, juce::Colour accent, float titleSize = 15.0f)
    {
        titleLabel.setText (title, juce::dontSendNotification);
        titleLabel.setJustificationType (juce::Justification::centred);
        titleLabel.setFont (juce::Font (juce::FontOptions (titleSize, juce::Font::bold)));
        titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xfff7e3c4));
        addAndMakeVisible (titleLabel);

        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 96, 20);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xfff7e3c4));
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (slider);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId, slider);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        titleLabel.setBounds (r.removeFromTop (juce::jmax (16, getHeight() / 8)));
        slider.setBounds (r);
    }

private:
    juce::Label titleLabel;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
class LaPautaVerbAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit LaPautaVerbAudioProcessorEditor (LaPautaVerbAudioProcessor&);
    ~LaPautaVerbAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void changePreset (int delta);
    void updateTexts();

    static constexpr float designW = 1010.0f;
    static constexpr float designH = 460.0f;

    struct Panel { juce::String name; juce::Rectangle<float> rect; juce::Colour colour; };
    std::vector<Panel> panels;

    LaPautaVerbAudioProcessor& processor;
    LaPautaLookAndFeel lnf;

    KnobCell mix, predelay, decay,
             highFreq, highShelf, bassFreq, bassMult,
             size, attack, earlyDiff, lateDiff,
             modRate, modDepth, highCut, lowCut;

    juce::Label modeLabel, colorLabel, presetLabel, presetName, description;
    juce::ComboBox modeBox, colorBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttach, colorAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LaPautaVerbAudioProcessorEditor)
};
