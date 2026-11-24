#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
struct Preset
{
    const char* name;
    float mix, predelay, decay, highFreq, highShelf, bassFreq, bassMult;
    float size, attack, earlyDiff, lateDiff, modRate, modDepth, highCut, lowCut;
    int mode, color;
};

const Preset presets[] = {
    { "Default",     50, 20, 4.0f, 6000, -24,  700, 1.5f, 100, 50, 100, 100, 2.5f, 38,  8000,  10, 0, 0 },
    { "Voz Urbana",  18, 80, 2.2f, 7500, -22,  450, 0.9f,  80, 65, 100, 100, 1.8f, 28, 10000, 150, 1, 0 },
    { "Plate Vocal", 25, 15, 1.8f, 9000, -18,  400, 0.8f,  70, 70, 100, 100, 1.5f, 20, 12000, 120, 2, 0 },
    { "Big Hall",    35, 35, 6.5f, 5000, -30,  600, 1.4f, 100, 40, 100, 100, 1.2f, 40,  9000,  60, 0, 0 },
    { "Dark Room",   20,  5, 0.9f, 3500, -30,  500, 1.0f,  60, 80,  80,  90, 2.0f, 25,  6000,  80, 3, 1 },
    { "Cathedral",   40, 50, 12.0f,4000, -36,  800, 1.8f, 100, 25, 100, 100, 0.8f, 50,  7000, 100, 5, 0 },
    { "Chamber 80s", 30, 12, 2.6f, 7000, -20,  500, 1.2f,  85, 60,  90, 100, 3.0f, 30, 11000, 100, 4, 2 }
};
constexpr int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));

std::unique_ptr<juce::AudioParameterFloat> makeFloat (const juce::String& id, const juce::String& name,
                                                      juce::NormalisableRange<float> range, float def,
                                                      const juce::String& suffix, int decimals)
{
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, 1 }, name, range, def,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([suffix, decimals] (float v, int) { return juce::String (v, decimals) + suffix; })
            .withValueFromStringFunction ([] (const juce::String& t) { return t.retainCharacters ("0123456789.-").getFloatValue(); }));
}

void setParam (juce::AudioProcessorValueTreeState& apvts, const char* id, float value)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}
} // namespace

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout LaPautaVerbAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (makeFloat ("mix",       "Mix",       { 0.0f, 100.0f, 0.1f },          50.0f,   " %",  1));
    layout.add (makeFloat ("predelay",  "Predelay",  { 0.0f, 500.0f, 0.01f, 0.5f },   20.0f,   " ms", 2));
    layout.add (makeFloat ("decay",     "Decay",     { 0.2f, 30.0f, 0.01f, 0.4f },    4.0f,    " s",  2));
    layout.add (makeFloat ("highFreq",  "HighFreq",  { 1000.0f, 20000.0f, 1.0f, 0.4f }, 6000.0f, " Hz", 0));
    layout.add (makeFloat ("highShelf", "HighShelf", { -60.0f, 0.0f, 0.01f },         -24.0f,  " dB", 2));
    layout.add (makeFloat ("bassFreq",  "BassFreq",  { 50.0f, 1000.0f, 1.0f, 0.5f },  700.0f,  " Hz", 0));
    layout.add (makeFloat ("bassMult",  "BassMult",  { 0.25f, 4.0f, 0.01f, 0.5f },    1.5f,    " X",  2));
    layout.add (makeFloat ("size",      "Size",      { 0.0f, 100.0f, 0.1f },          100.0f,  " %",  1));
    layout.add (makeFloat ("attack",    "Attack",    { 0.0f, 100.0f, 0.1f },          50.0f,   " %",  1));
    layout.add (makeFloat ("earlyDiff", "Early",     { 0.0f, 100.0f, 0.1f },          100.0f,  " %",  1));
    layout.add (makeFloat ("lateDiff",  "Late",      { 0.0f, 100.0f, 0.1f },          100.0f,  " %",  1));
    layout.add (makeFloat ("modRate",   "Rate",      { 0.1f, 10.0f, 0.01f, 0.5f },    2.5f,    " Hz", 2));
    layout.add (makeFloat ("modDepth",  "Depth",     { 0.0f, 100.0f, 0.1f },          38.0f,   " %",  1));
    layout.add (makeFloat ("highCut",   "HighCut",   { 1000.0f, 20000.0f, 1.0f, 0.4f }, 8000.0f, " Hz", 0));
    layout.add (makeFloat ("lowCut",    "LowCut",    { 10.0f, 1000.0f, 1.0f, 0.4f },  10.0f,   " Hz", 0));

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mode", 1 },  "Mode",  pauta::getModeNames(),  0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "color", 1 }, "Color", pauta::getColorNames(), 0));

    return layout;
}

//==============================================================================
LaPautaVerbAudioProcessor::LaPautaVerbAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createParameterLayout())
{
    pMix       = apvts.getRawParameterValue ("mix");
    pPredelay  = apvts.getRawParameterValue ("predelay");
    pDecay     = apvts.getRawParameterValue ("decay");
    pHighFreq  = apvts.getRawParameterValue ("highFreq");
    pHighShelf = apvts.getRawParameterValue ("highShelf");
    pBassFreq  = apvts.getRawParameterValue ("bassFreq");
    pBassMult  = apvts.getRawParameterValue ("bassMult");
    pSize      = apvts.getRawParameterValue ("size");
    pAttack    = apvts.getRawParameterValue ("attack");
    pEarlyDiff = apvts.getRawParameterValue ("earlyDiff");
    pLateDiff  = apvts.getRawParameterValue ("lateDiff");
    pModRate   = apvts.getRawParameterValue ("modRate");
    pModDepth  = apvts.getRawParameterValue ("modDepth");
    pHighCut   = apvts.getRawParameterValue ("highCut");
    pLowCut    = apvts.getRawParameterValue ("lowCut");
    pMode      = apvts.getRawParameterValue ("mode");
    pColor     = apvts.getRawParameterValue ("color");
}

void LaPautaVerbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    monoScratch.assign ((size_t) std::max (1, samplesPerBlock), 0.0f);
}

bool LaPautaVerbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void LaPautaVerbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    for (int ch = getTotalNumInputChannels(); ch < numCh; ++ch)
        buffer.clear (ch, 0, n);
    if (n <= 0 || numCh < 1)
        return;

    pauta::ReverbParams rp;
    rp.mix         = pMix->load() / 100.0f;
    rp.predelayMs  = pPredelay->load();
    rp.decay       = pDecay->load();
    rp.highFreq    = pHighFreq->load();
    rp.highShelfDb = pHighShelf->load();
    rp.bassFreq    = pBassFreq->load();
    rp.bassMult    = pBassMult->load();
    rp.size        = pSize->load() / 100.0f;
    rp.attack      = pAttack->load() / 100.0f;
    rp.earlyDiff   = pEarlyDiff->load() / 100.0f;
    rp.lateDiff    = pLateDiff->load() / 100.0f;
    rp.modRate     = pModRate->load();
    rp.modDepth    = pModDepth->load() / 100.0f;
    rp.highCut     = pHighCut->load();
    rp.lowCut      = pLowCut->load();
    rp.mode        = (int) pMode->load();
    rp.color       = (int) pColor->load();
    engine.setParams (rp);

    float* L = buffer.getWritePointer (0);

    if (numCh > 1)
    {
        engine.process (L, buffer.getWritePointer (1), n);
    }
    else
    {
        // mono: se duplica el canal para el motor y se promedia a la salida
        const int chunk = (int) monoScratch.size();
        for (int start = 0; start < n; start += chunk)
        {
            const int len = std::min (chunk, n - start);
            std::copy (L + start, L + start + len, monoScratch.begin());
            engine.process (L + start, monoScratch.data(), len);
            for (int i = 0; i < len; ++i)
                L[start + i] = 0.5f * (L[start + i] + monoScratch[(size_t) i]);
        }
    }
}

//==============================================================================
int LaPautaVerbAudioProcessor::getNumPrograms() { return numPresets; }

const juce::String LaPautaVerbAudioProcessor::getProgramName (int index)
{
    return presets[juce::jlimit (0, numPresets - 1, index)].name;
}

void LaPautaVerbAudioProcessor::setCurrentProgram (int index)
{
    currentProgram = juce::jlimit (0, numPresets - 1, index);
    const auto& p = presets[currentProgram];

    setParam (apvts, "mix", p.mix);
    setParam (apvts, "predelay", p.predelay);
    setParam (apvts, "decay", p.decay);
    setParam (apvts, "highFreq", p.highFreq);
    setParam (apvts, "highShelf", p.highShelf);
    setParam (apvts, "bassFreq", p.bassFreq);
    setParam (apvts, "bassMult", p.bassMult);
    setParam (apvts, "size", p.size);
    setParam (apvts, "attack", p.attack);
    setParam (apvts, "earlyDiff", p.earlyDiff);
    setParam (apvts, "lateDiff", p.lateDiff);
    setParam (apvts, "modRate", p.modRate);
    setParam (apvts, "modDepth", p.modDepth);
    setParam (apvts, "highCut", p.highCut);
    setParam (apvts, "lowCut", p.lowCut);
    setParam (apvts, "mode", (float) p.mode);
    setParam (apvts, "color", (float) p.color);
}

//==============================================================================
void LaPautaVerbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void LaPautaVerbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = juce::jlimit (0, numPresets - 1, (int) tree.getProperty ("program", 0));
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* LaPautaVerbAudioProcessor::createEditor()
{
    return new LaPautaVerbAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LaPautaVerbAudioProcessor();
}
