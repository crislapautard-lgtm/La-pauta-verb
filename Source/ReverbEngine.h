#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

// Motor de reverb original de La Pauta Verb:
// red de retardos realimentados (FDN) de 8 lineas con matriz Hadamard,
// reflexiones tempranas, difusion, modulacion, amortiguacion de agudos/graves.
namespace pauta
{
inline juce::StringArray getModeNames()  { return { "Concert Hall", "Bright Hall", "Plate", "Room", "Chamber", "Cathedral" }; }
inline juce::StringArray getColorNames() { return { "Modern", "1970s", "1980s" }; }

struct ModeInfo { float sizeMul, earlyLevel, earlyScale; };

inline ModeInfo getModeInfo (int i)
{
    static const ModeInfo modes[] = {
        { 1.00f, 0.50f, 1.00f },  // Concert Hall
        { 0.85f, 0.60f, 0.90f },  // Bright Hall
        { 0.70f, 0.15f, 0.30f },  // Plate
        { 0.45f, 0.90f, 0.60f },  // Room
        { 0.60f, 0.70f, 0.70f },  // Chamber
        { 1.50f, 0.30f, 1.60f }   // Cathedral
    };
    return modes[juce::jlimit (0, 5, i)];
}

struct ReverbParams
{
    float mix         = 0.5f;     // 0..1
    float predelayMs  = 20.0f;
    float decay       = 4.0f;     // segundos (RT60)
    float highFreq    = 6000.0f;  // Hz  - frecuencia de amortiguacion de agudos
    float highShelfDb = -24.0f;   // dB por segundo de cola sobre highFreq
    float bassFreq    = 700.0f;   // Hz  - frecuencia de graves
    float bassMult    = 1.5f;     // multiplicador de decay en graves
    float size        = 1.0f;     // 0..1
    float attack      = 0.5f;     // 0..1
    float earlyDiff   = 1.0f;     // 0..1
    float lateDiff    = 1.0f;     // 0..1
    float modRate     = 2.5f;     // Hz
    float modDepth    = 0.38f;    // 0..1
    float highCut     = 8000.0f;  // Hz
    float lowCut      = 10.0f;    // Hz
    int   mode        = 0;
    int   color       = 0;
};

//==============================================================================
class DelayLine
{
public:
    void prepare (int maxSamples)
    {
        int size = 16;
        while (size < maxSamples + 8) size <<= 1;
        buf.assign ((size_t) size, 0.0f);
        mask = size - 1;
        w = 0;
    }

    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }

    void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }

    // d = muestras de retardo (>= 1). read(1) devuelve la ultima muestra escrita.
    float read (float d) const
    {
        const float pos  = (float) w - d;
        const int   i    = (int) std::floor (pos);
        const float frac = pos - (float) i;
        const float a = buf[(size_t) (i & mask)];
        const float b = buf[(size_t) ((i + 1) & mask)];
        return a + frac * (b - a);
    }

private:
    std::vector<float> buf;
    int mask = 15;
    int w = 0;
};

class Allpass
{
public:
    void prepare (int len)
    {
        buf.assign ((size_t) std::max (1, len), 0.0f);
        idx = 0;
    }

    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); idx = 0; }

    float process (float x)
    {
        const float d = buf[(size_t) idx];
        const float v = x - g * d;
        const float y = d + g * v;
        buf[(size_t) idx] = v;
        if (++idx >= (int) buf.size()) idx = 0;
        return y;
    }

    float g = 0.0f;

private:
    std::vector<float> buf;
    int idx = 0;
};

//==============================================================================
class LaPautaReverb
{
public:
    static constexpr int N = 8;
    static constexpr int numTaps = 12;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        const double k = sr / 44100.0;

        preL.prepare ((int) (0.55 * sr));  preR.prepare ((int) (0.55 * sr));
        erL.prepare  ((int) (0.60 * sr));  erR.prepare  ((int) (0.60 * sr));
        lateL.prepare ((int) (0.25 * sr)); lateR.prepare ((int) (0.25 * sr));

        const int maxLine = (int) (2953.0 * k * 2.0) + 512;
        static const int apLine44[N] = { 113, 149, 181, 211, 241, 271, 307, 337 };
        for (int i = 0; i < N; ++i)
        {
            line[i].prepare (maxLine);
            apLen[i] = std::max (2, (int) std::lround ((double) apLine44[i] * k));
            lineAp[(size_t) i].prepare (apLen[i]);
        }

        static const int eL44[4] = { 142, 107, 379, 277 };
        static const int eR44[4] = { 150, 113, 391, 283 };
        for (int i = 0; i < 4; ++i)
        {
            earlyApL[(size_t) i].prepare (std::max (2, (int) std::lround ((double) eL44[i] * k)));
            earlyApR[(size_t) i].prepare (std::max (2, (int) std::lround ((double) eR44[i] * k)));
        }

        static const float tapMsL[numTaps] = { 7, 11, 17, 23, 31, 41, 53, 67, 79, 97, 113, 131 };
        static const float tapMsR[numTaps] = { 9, 13, 19, 29, 37, 47, 59, 71, 89, 103, 121, 139 };
        for (int t = 0; t < numTaps; ++t)
        {
            tapMsLArr[(size_t) t] = tapMsL[t];
            tapMsRArr[(size_t) t] = tapMsR[t];
            const float sign = (t % 2 == 0) ? 1.0f : -1.0f;
            tapGainL[(size_t) t] = sign * 0.9f * std::exp (-tapMsL[t] / 90.0f);
            tapGainR[(size_t) t] = -sign * 0.9f * std::exp (-tapMsR[t] / 90.0f);
        }

        mixSm.reset (sr, 0.03);
        mixSm.setCurrentAndTargetValue (0.5f);
        smoothCoef = 1.0f - std::exp (-1.0f / (0.04f * (float) sr));

        reset();
        needsInit = true;
    }

    void reset()
    {
        preL.clear(); preR.clear(); erL.clear(); erR.clear(); lateL.clear(); lateR.clear();
        for (auto& l : line) l.clear();
        for (auto& a : lineAp) a.clear();
        for (auto& a : earlyApL) a.clear();
        for (auto& a : earlyApR) a.clear();
        for (int i = 0; i < N; ++i) { lpHi[i] = 0.0f; lpBass[i] = 0.0f; noise[i] = 0.0f; }
        hcL1 = hcL2 = hcR1 = hcR2 = 0.0f;
        lcL1 = lcL2 = lcR1 = lcR2 = 0.0f;
        coL1 = coL2 = coR1 = coR2 = 0.0f;
        phase = 0.0f;
    }

    void setParams (const ReverbParams& p)
    {
        params = p;
        const ModeInfo mi = getModeInfo (p.mode);
        const float k = (float) (sr / 44100.0);

        static const float base44[N] = { 1031, 1327, 1523, 1789, 2111, 2347, 2677, 2953 };
        const float sizeScale = mi.sizeMul * (0.25f + 0.75f * p.size);
        const float bassMult  = juce::jlimit (0.25f, 4.0f, p.bassMult);
        const float decay     = std::max (0.1f, p.decay);

        float gSum = 0.0f;
        for (int i = 0; i < N; ++i)
        {
            tgtDelay[i] = base44[i] * k * sizeScale;
            const float loopSec = (tgtDelay[i] + (float) apLen[i]) / (float) sr;
            gDecay[i] = std::pow (10.0f, -3.0f * loopSec / decay);
            gHigh[i]  = std::pow (10.0f, (p.highShelfDb / 20.0f) * loopSec);
            rBass[i]  = std::pow (gDecay[i], 1.0f / bassMult - 1.0f);
            gSum += gDecay[i];
        }
        const float gMean = gSum / (float) N;
        inputGain = std::sqrt (std::max (0.02f, 1.0f - gMean * gMean));

        aHi   = onePole (std::min (p.highFreq, 0.45f * (float) sr));
        aBass = onePole (std::min (p.bassFreq, 0.45f * (float) sr));
        aHC   = onePole (std::min (p.highCut,  0.45f * (float) sr));
        aLC   = onePole (std::min (p.lowCut,   0.45f * (float) sr));

        if (p.color == 1)      aCol = onePole (std::min (9000.0f,  0.45f * (float) sr));
        else if (p.color == 2) aCol = onePole (std::min (15000.0f, 0.45f * (float) sr));
        else                   aCol = 1.0f;

        const float apG = 0.62f * p.lateDiff;
        for (auto& a : lineAp) a.g = apG;
        const float eG = 0.72f * p.earlyDiff;
        for (auto& a : earlyApL) a.g = eG;
        for (auto& a : earlyApR) a.g = eG;

        const float erScale = mi.earlyScale * (0.4f + 0.6f * p.size);
        for (int t = 0; t < numTaps; ++t)
        {
            tapDelL[(size_t) t] = tapMsLArr[(size_t) t] * 0.001f * (float) sr * erScale + 1.0f;
            tapDelR[(size_t) t] = tapMsRArr[(size_t) t] * 0.001f * (float) sr * erScale + 1.0f;
        }
        earlyGain = mi.earlyLevel * (0.4f + 0.6f * p.attack);

        preTarget  = std::max (1.0f, p.predelayMs * 0.001f * (float) sr);
        lateTarget = (1.0f - p.attack) * 0.08f * (float) sr;

        modSamples  = p.modDepth * 14.0f * k;
        modSamplesMax = 14.0f * k;
        phaseInc    = p.modRate / (float) sr;

        mixSm.setTargetValue (p.mix);

        if (needsInit)
        {
            for (int i = 0; i < N; ++i) curDelay[i] = tgtDelay[i];
            preCur = preTarget;
            lateCur = lateTarget;
            mixSm.setCurrentAndTargetValue (p.mix);
            needsInit = false;
        }
    }

    void process (float* L, float* R, int numSamples)
    {
        const float twoPi = juce::MathConstants<float>::twoPi;

        for (int s = 0; s < numSamples; ++s)
        {
            const float inL = L[s];
            const float inR = R[s];

            preCur  += smoothCoef * (preTarget  - preCur);
            lateCur += smoothCoef * (lateTarget - lateCur);

            // predelay
            const float pl = preL.read (std::max (1.0f, preCur));
            const float pr = preR.read (std::max (1.0f, preCur));
            preL.push (inL);
            preR.push (inR);

            // difusion temprana
            float dl = pl, dr = pr;
            for (auto& a : earlyApL) dl = a.process (dl);
            for (auto& a : earlyApR) dr = a.process (dr);

            // reflexiones tempranas
            erL.push (dl);
            erR.push (dr);
            float eOutL = 0.0f, eOutR = 0.0f;
            for (int t = 0; t < numTaps; ++t)
            {
                eOutL += tapGainL[(size_t) t] * erL.read (tapDelL[(size_t) t]);
                eOutR += tapGainR[(size_t) t] * erR.read (tapDelR[(size_t) t]);
            }
            eOutL *= 0.45f;
            eOutR *= 0.45f;

            // entrada a la cola (retraso segun Attack)
            lateL.push (dl);
            lateR.push (dr);
            const float ll = lateL.read (std::max (1.0f, lateCur + 1.0f));
            const float lr = lateR.read (std::max (1.0f, lateCur + 1.0f));

            // FDN
            float t[N];
            for (int i = 0; i < N; ++i)
            {
                curDelay[i] += smoothCoef * (tgtDelay[i] - curDelay[i]);

                float m = std::sin (twoPi * (phase + (float) i * 0.125f)) * modSamples;
                if (params.color == 1)
                {
                    noise[i] += 0.002f * ((rng.nextFloat() * 2.0f - 1.0f) - noise[i]);
                    m += noise[i] * modSamplesMax * 10.0f;
                }

                float y = line[i].read (std::max (2.0f, curDelay[i] + m));

                // amortiguacion de agudos
                lpHi[i] += aHi * (y - lpHi[i]);
                const float hi = y - lpHi[i];
                const float y2 = lpHi[i] + hi * gHigh[i];

                // control de graves + ganancia de decay
                lpBass[i] += aBass * (y2 - lpBass[i]);
                const float y3 = (y2 + lpBass[i] * (rBass[i] - 1.0f)) * gDecay[i];

                t[i] = lineAp[(size_t) i].process (y3);
            }

            const float outL = 0.5f * (t[0] - t[2] + t[4] - t[6]);
            const float outR = 0.5f * (t[1] - t[3] + t[5] - t[7]);

            // matriz Hadamard (Walsh-Hadamard rapida de 8)
            for (int len = 1; len < N; len <<= 1)
                for (int i = 0; i < N; i += len << 1)
                    for (int j = i; j < i + len; ++j)
                    {
                        const float a = t[j], b = t[j + len];
                        t[j] = a + b;
                        t[j + len] = a - b;
                    }

            for (int i = 0; i < N; ++i)
            {
                const float in   = (i & 1) ? lr : ll;
                const float sign = ((i >> 1) & 1) ? -1.0f : 1.0f;
                line[i].push (t[i] * 0.35355339f + in * sign * inputGain);
            }

            phase += phaseInc;
            if (phase >= 1.0f) phase -= 1.0f;

            float wetL = outL + earlyGain * eOutL;
            float wetR = outR + earlyGain * eOutR;

            // EQ: High Cut (2 polos)
            hcL1 += aHC * (wetL - hcL1);  hcL2 += aHC * (hcL1 - hcL2);  wetL = hcL2;
            hcR1 += aHC * (wetR - hcR1);  hcR2 += aHC * (hcR1 - hcR2);  wetR = hcR2;

            // EQ: Low Cut (2 polos)
            lcL1 += aLC * (wetL - lcL1);  wetL -= lcL1;
            lcL2 += aLC * (wetL - lcL2);  wetL -= lcL2;
            lcR1 += aLC * (wetR - lcR1);  wetR -= lcR1;
            lcR2 += aLC * (wetR - lcR2);  wetR -= lcR2;

            // Color
            if (params.color != 0)
            {
                coL1 += aCol * (wetL - coL1);  coL2 += aCol * (coL1 - coL2);  wetL = coL2;
                coR1 += aCol * (wetR - coR1);  coR2 += aCol * (coR1 - coR2);  wetR = coR2;
            }

            const float mx = mixSm.getNextValue();
            L[s] = inL * (1.0f - mx) + wetL * mx;
            R[s] = inR * (1.0f - mx) + wetR * mx;
        }
    }

private:
    float onePole (float fc) const
    {
        return 1.0f - std::exp (-juce::MathConstants<float>::twoPi * std::max (1.0f, fc) / (float) sr);
    }

    double sr = 44100.0;
    ReverbParams params;
    bool needsInit = true;
    float smoothCoef = 0.001f;

    DelayLine preL, preR, erL, erR, lateL, lateR;
    DelayLine line[N];
    std::array<Allpass, N> lineAp;
    std::array<Allpass, 4> earlyApL, earlyApR;
    int apLen[N] {};

    float tgtDelay[N] {}, curDelay[N] {};
    float gDecay[N] {}, gHigh[N] {}, rBass[N] {};
    float lpHi[N] {}, lpBass[N] {}, noise[N] {};
    float inputGain = 0.3f;
    float aHi = 0.1f, aBass = 0.01f, aHC = 0.5f, aLC = 0.001f, aCol = 1.0f;

    std::array<float, numTaps> tapMsLArr {}, tapMsRArr {}, tapGainL {}, tapGainR {}, tapDelL {}, tapDelR {};
    float earlyGain = 0.3f;

    float preCur = 1.0f, preTarget = 1.0f, lateCur = 0.0f, lateTarget = 0.0f;
    float modSamples = 0.0f, modSamplesMax = 14.0f, phase = 0.0f, phaseInc = 0.0f;

    float hcL1 = 0, hcL2 = 0, hcR1 = 0, hcR2 = 0;
    float lcL1 = 0, lcL2 = 0, lcR1 = 0, lcR2 = 0;
    float coL1 = 0, coL2 = 0, coR1 = 0, coR2 = 0;

    juce::SmoothedValue<float> mixSm;
    juce::Random rng;
};
} // namespace pauta
