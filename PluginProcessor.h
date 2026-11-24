#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ReverbEngine.h"

class LaPautaVerbAudioProcessor : public juce::AudioProcessor
{
public:
    LaPautaVerbAudioProcessor();
    ~LaPautaVerbAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "La Pauta Verb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 30.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    pauta::LaPautaReverb engine;
    std::vector<float> monoScratch;
    int currentProgram = 0;

    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pPredelay = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pHighFreq = nullptr;
    std::atomic<float>* pHighShelf = nullptr;
    std::atomic<float>* pBassFreq = nullptr;
    std::atomic<float>* pBassMult = nullptr;
    std::atomic<float>* pSize = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pEarlyDiff = nullptr;
    std::atomic<float>* pLateDiff = nullptr;
    std::atomic<float>* pModRate = nullptr;
    std::atomic<float>* pModDepth = nullptr;
    std::atomic<float>* pHighCut = nullptr;
    std::atomic<float>* pLowCut = nullptr;
    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pColor = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LaPautaVerbAudioProcessor)
};
