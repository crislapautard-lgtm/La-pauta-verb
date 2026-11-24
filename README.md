# La Pauta Verb

Reverb VST3 para Windows (JUCE + CMake), con controles inspirados en los reverbs de sala/placa clasicos:
Mix, Predelay, Decay, Damping (HighFreq, HighShelf, BassFreq, BassMult), Shape (Size, Attack),
Diff (Early, Late), Mod (Rate, Depth), EQ (HighCut, LowCut), Mode y Color, mas 7 presets.

El motor de reverb es original (red de retardos realimentados de 8 lineas con matriz Hadamard);
no es una copia de ningun otro plugin, asi que suena parecido en concepto pero no identico.

## Como compilarlo (sin instalar nada: GitHub lo hace por ti)

1. Crea un repositorio nuevo en GitHub llamado `La-Pauta-Verb`.
2. Sube **todo el contenido de esta carpeta** (incluida la carpeta oculta `.github`).
3. Entra a la pestana **Actions**. El workflow "Build La Pauta Verb (Windows VST3)" corre solo
   con cada push (o dale a *Run workflow*).
4. Cuando termine (unos 5-10 min), abre la ejecucion y descarga el artefacto
   **La-Pauta-Verb-VST3-Windows**.
5. Descomprime y copia la carpeta `La Pauta Verb.vst3` a:
   `C:\Program Files\Common Files\VST3\`
6. Reescanea plugins en tu DAW.

## Compilar en tu PC (opcional)

Requisitos: Visual Studio 2022 (Desktop C++) y CMake.

```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

El plugin queda en `build/LaPautaVerb_artefacts/Release/VST3/`.

## Estructura

- `CMakeLists.txt` - proyecto (descarga JUCE 8.0.4 automaticamente)
- `Source/ReverbEngine.h` - motor de reverb
- `Source/PluginProcessor.*` - parametros, presets, audio
- `Source/PluginEditor.*` - interfaz grafica
- `.github/workflows/build.yml` - compilacion automatica en GitHub

## Ajustes rapidos del sonido

- Decay largo + BassMult > 1 = graves que duran mas.
- HighShelf mas negativo = agudos que mueren antes (cola mas oscura).
- Attack bajo = la cola entra mas tarde y con mas pre-espacio.
- Color 1970s = mas oscuro y con ruido en la modulacion.
