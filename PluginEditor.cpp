#include "PluginEditor.h"

namespace
{
const juce::Colour cGold   (0xfff0b25c);
const juce::Colour cOrange (0xffe8803f);
const juce::Colour cCoral  (0xffd9594f);
const juce::Colour cRose   (0xffc9587a);
const juce::Colour cRed    (0xffb83a30);

const char* modeDescriptions[] = {
    "Concert Hall: sala grande, ataque lento y cola amplia",
    "Bright Hall: sala brillante, ataque mas rapido",
    "Plate: placa densa, casi sin reflexiones tempranas",
    "Room: cuarto pequeno, reflexiones tempranas marcadas",
    "Chamber: camara mediana con cuerpo calido",
    "Cathedral: espacio enorme, cola muy larga"
};

const char* colorDescriptions[] = {
    "Modern: ancho de banda completo, limpio",
    "1970s: oscuro, modulacion con ruido",
    "1980s: ligeramente opaco, estilo digital clasico"
};
}

//==============================================================================
LaPautaVerbAudioProcessorEditor::LaPautaVerbAudioProcessorEditor (LaPautaVerbAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      mix       (p.apvts, "mix",       "MIX",       cOrange, 15.0f),
      predelay  (p.apvts, "predelay",  "PREDELAY",  cOrange, 15.0f),
      decay     (p.apvts, "decay",     "DECAY",     cGold,   20.0f),
      highFreq  (p.apvts, "highFreq",  "HighFreq",  cGold),
      highShelf (p.apvts, "highShelf", "HighShelf", cGold),
      bassFreq  (p.apvts, "bassFreq",  "BassFreq",  cGold),
      bassMult  (p.apvts, "bassMult",  "BassMult",  cGold),
      size      (p.apvts, "size",      "Size",      cGold),
      attack    (p.apvts, "attack",    "Attack",    cGold),
      earlyDiff (p.apvts, "earlyDiff", "Early",     cOrange),
      lateDiff  (p.apvts, "lateDiff",  "Late",      cOrange),
      modRate   (p.apvts, "modRate",   "Rate",      cRose),
      modDepth  (p.apvts, "modDepth",  "Depth",     cRose),
      highCut   (p.apvts, "highCut",   "HighCut",   cRed),
      lowCut    (p.apvts, "lowCut",    "LowCut",    cRed)
{
    setLookAndFeel (&lnf);

    panels = {
        { "DAMPING", { 345, 100, 220, 260 }, cGold   },
        { "SHAPE",   { 575, 100,  95, 260 }, cGold   },
        { "DIFF",    { 680, 100,  95, 260 }, cOrange },
        { "MOD",     { 785, 100,  95, 260 }, cRose   },
        { "EQ",      { 890, 100,  95, 260 }, cRed    }
    };

    for (auto* c : { &mix, &predelay, &decay, &highFreq, &highShelf, &bassFreq, &bassMult,
                     &size, &attack, &earlyDiff, &lateDiff, &modRate, &modDepth, &highCut, &lowCut })
        addAndMakeVisible (*c);

    auto styleLabel = [] (juce::Label& l, const juce::String& text, float size, juce::Justification j)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (juce::Font (juce::FontOptions (size, juce::Font::bold)));
        l.setColour (juce::Label::textColourId, juce::Colour (0xfff7e3c4));
        l.setJustificationType (j);
    };

    styleLabel (modeLabel,   "MODE:",    16.0f, juce::Justification::centredRight);
    styleLabel (colorLabel,  "COLOR:",   16.0f, juce::Justification::centredRight);
    styleLabel (presetLabel, "PRESETS:", 16.0f, juce::Justification::centredRight);
    styleLabel (presetName,  "",         16.0f, juce::Justification::centred);
    styleLabel (description, "",         14.0f, juce::Justification::centredLeft);
    presetName.setColour (juce::Label::textColourId, cGold);
    description.setColour (juce::Label::textColourId, juce::Colour (0xffe9cfa8));

    for (auto* l : { &modeLabel, &colorLabel, &presetLabel, &presetName, &description })
        addAndMakeVisible (*l);

    const auto modes = pauta::getModeNames();
    for (int i = 0; i < modes.size(); ++i) modeBox.addItem (modes[i], i + 1);
    const auto colors = pauta::getColorNames();
    for (int i = 0; i < colors.size(); ++i) colorBox.addItem (colors[i], i + 1);
    addAndMakeVisible (modeBox);
    addAndMakeVisible (colorBox);
    modeAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "mode",  modeBox);
    colorAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "color", colorBox);

    prevButton.onClick = [this] { changePreset (-1); };
    nextButton.onClick = [this] { changePreset (+1); };
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);

    setResizable (true, true);
    setResizeLimits ((int) (designW * 0.75f), (int) (designH * 0.75f), (int) (designW * 1.5f), (int) (designH * 1.5f));
    getConstrainer()->setFixedAspectRatio ((double) (designW / designH));
    setSize ((int) designW, (int) designH);

    updateTexts();
    startTimerHz (8);
}

LaPautaVerbAudioProcessorEditor::~LaPautaVerbAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void LaPautaVerbAudioProcessorEditor::changePreset (int delta)
{
    const int n = processor.getNumPrograms();
    processor.setCurrentProgram ((processor.getCurrentProgram() + delta + n) % n);
    updateTexts();
}

void LaPautaVerbAudioProcessorEditor::updateTexts()
{
    presetName.setText (processor.getProgramName (processor.getCurrentProgram()), juce::dontSendNotification);

    const int m = juce::jlimit (0, 5, modeBox.getSelectedItemIndex());
    const int c = juce::jlimit (0, 2, colorBox.getSelectedItemIndex());
    description.setText (juce::String (modeDescriptions[m]) + "\n" + colorDescriptions[c], juce::dontSendNotification);
}

void LaPautaVerbAudioProcessorEditor::timerCallback() { updateTexts(); }

//==============================================================================
void LaPautaVerbAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float s = (float) getWidth() / designW;

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a1a1c), 0.0f, 0.0f,
                                             juce::Colour (0xff1a0a0d), (float) getWidth(), (float) getHeight(), false));
    g.fillAll();

    // titulo con degradado
    {
        juce::Font f (juce::FontOptions (46.0f * s, juce::Font::bold));
        g.setFont (f);
        g.setGradientFill (juce::ColourGradient (cGold, 30.0f * s, 20.0f * s, cRose, 360.0f * s, 20.0f * s, false));
        g.drawText ("La Pauta Verb", juce::Rectangle<float> (30.0f * s, 18.0f * s, 400.0f * s, 56.0f * s),
                    juce::Justification::centredLeft);

        g.setColour (juce::Colour (0xffe9cfa8));
        g.setFont (juce::Font (juce::FontOptions (14.0f * s, juce::Font::bold)));
        g.drawText ("Version 1.0.0  VST3", juce::Rectangle<float> (760.0f * s, 24.0f * s, 225.0f * s, 20.0f * s),
                    juce::Justification::centredRight);
    }

    for (auto& p : panels)
    {
        auto r = juce::Rectangle<float> (p.rect.getX() * s, p.rect.getY() * s, p.rect.getWidth() * s, p.rect.getHeight() * s);
        g.setColour (juce::Colour (0x33000000));
        g.fillRoundedRectangle (r, 22.0f * s);
        g.setColour (p.colour);
        g.drawRoundedRectangle (r.reduced (1.0f), 22.0f * s, 2.0f);

        g.setColour (juce::Colour (0xfff7e3c4));
        g.setFont (juce::Font (juce::FontOptions (17.0f * s, juce::Font::bold)));
        g.drawText (p.name, r.withHeight (30.0f * s).translated (0.0f, 4.0f * s), juce::Justification::centred);
    }
}

void LaPautaVerbAudioProcessorEditor::resized()
{
    const float s = (float) getWidth() / designW;

    auto place = [s] (juce::Component& c, float x, float y, float w, float h)
    {
        c.setBounds ((int) std::round (x * s), (int) std::round (y * s),
                     (int) std::round (w * s), (int) std::round (h * s));
    };

    place (mix,      20, 100, 100, 130);
    place (predelay, 20, 230, 100, 130);
    place (decay,   125, 100, 210, 260);

    place (highFreq,  350, 130, 105, 115);
    place (highShelf, 455, 130, 105, 115);
    place (bassFreq,  350, 245, 105, 115);
    place (bassMult,  455, 245, 105, 115);

    place (size,   575, 130, 95, 115);
    place (attack, 575, 245, 95, 115);

    place (earlyDiff, 680, 130, 95, 115);
    place (lateDiff,  680, 245, 95, 115);

    place (modRate,  785, 130, 95, 115);
    place (modDepth, 785, 245, 95, 115);

    place (highCut, 890, 130, 95, 115);
    place (lowCut,  890, 245, 95, 115);

    place (modeLabel,   20, 378,  70, 30);
    place (modeBox,     95, 378, 170, 30);
    place (colorLabel, 275, 378,  80, 30);
    place (colorBox,   360, 378, 130, 30);

    place (presetLabel, 520, 378, 100, 30);
    place (prevButton,  625, 378,  34, 30);
    place (presetName,  664, 378, 200, 30);
    place (nextButton,  869, 378,  34, 30);

    place (description, 30, 416, 940, 40);

    const float fs = 14.0f * s;
    description.setFont (juce::Font (juce::FontOptions (fs, juce::Font::bold)));
}
