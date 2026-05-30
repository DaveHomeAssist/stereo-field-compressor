# Stereo Field Compressor

JUCE/C++ VST3 plugin that compresses based on stereo field position rather than frequency or mid/side. Sidechain-driven spatial compression using a raised-cosine cone window.

**Status:** Scaffold ready to push. Core DSP math is implemented. UI is minimal but functional. Not yet tuned for musical results — tuning pass required before release.

- Concept: Connor Altizer
- Build: Dave Robertson
- Forked from: `bpm-delay-calculator` shell

---

## Pushing to GitHub

From this directory, on Dave's machine:

```bash
cd "~/path-to/stereo-field-compressor"
git init
git add .
git commit -m "Initial scaffold: JUCE VST3 with raised-cosine cone compressor"
gh repo create DaveHomeAssist/stereo-field-compressor --public --source=. --push
```

If `gh` is not installed:

```bash
git init
git add .
git commit -m "Initial scaffold: JUCE VST3 with raised-cosine cone compressor"
git remote add origin git@github.com:DaveHomeAssist/stereo-field-compressor.git
git branch -M main
git push -u origin main
```

(Create the empty `stereo-field-compressor` repo at github.com/DaveHomeAssist first.)

---

## Building

**Prerequisites:** CMake ≥ 3.22, a C++17 compiler, JUCE 7.x.

JUCE is pulled automatically via `FetchContent`. First configure takes a few minutes; afterward, incremental builds are fast.

```bash
cmake -B build -S .
cmake --build build --config Release
```

The VST3 output lands in `build/StereoFieldCompressor_artefacts/Release/VST3/`.

### Apple Silicon

Default is `arm64`. Add `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` for a universal binary.

---

## What's production-ready

- CMake build chain with JUCE FetchContent — builds clean on macOS, Windows, Linux.
- `SpatialDetector` — per-sample stereo field position estimator, returns angle in `[-π/2, π/2]` (hard-L to hard-R).
- `ConeWindow` — raised-cosine gain curve centered on an arbitrary angle with adjustable width. Equation: `0.5 * (1 + cos(π * (θ - θ_center) / θ_width))` clamped to `[0, 1]`.
- `ConeCompressor` — the actual compression stage, feed-forward topology, per-sample attack/release, ratio + threshold + knee.
- Parameter tree (`juce::AudioProcessorValueTreeState`) with automatable params: Threshold, Ratio, Attack, Release, Cone Center, Cone Width, Makeup, Mix.
- Preset save/load via JUCE's state API.
- Plugin identifiers set for Dave — change the PLUGIN_MANUFACTURER_CODE + PLUGIN_CODE in `CMakeLists.txt` before any release.

## What's NOT production-ready (gaps)

These are explicit and require attention before shipping:

1. **Sidechain routing** — scaffold accepts a sidechain bus, but the detector currently runs on the main input. Switch to reading the sidechain when available. See `PluginProcessor.cpp:processBlock` — marked `TODO(sidechain)`.
2. **Tuning** — attack/release curves are conservative defaults (RC one-pole). Musical tuning (program-dependent release, auto-gain) not done.
3. **UI** — current editor is generic sliders. No spatial visualization yet. You'll want a polar view showing cone position + input distribution.
4. **Latency reporting** — detector uses a 32-sample smoothing window but `getLatencySamples()` returns 0. Fix if any lookahead is added.
5. **Mid/Side bypass fallback** — no graceful behavior for mono-summed input. Currently detector returns 0 (center) for mono; verify this is musically acceptable.
6. **Oversampling** — not implemented. Add `juce::dsp::Oversampling` before the gain stage to avoid inter-sample peaks.
7. **Metering** — no GR meter. Add a `LevelMeter` widget and expose reduction from `ConeCompressor::getLastGainReductionDb()`.
8. **Tests** — no unit tests. At minimum, add unit tests for `ConeWindow::gainAt(angle)` boundary cases (center=0, width=π/2 full scale, etc.).
9. **AU / AAX** — CMake targets VST3 only. Add AU for macOS if needed; AAX requires separate SDK + NDA.
10. **CI** — no GitHub Actions. A matrix build on macos-latest / ubuntu-latest / windows-latest is straightforward.

## Next steps (prioritized)

1. Wire sidechain routing (~30 min).
2. Add polar spatial-view UI component (~3 hr).
3. Tune attack/release defaults with Connor on real material (~1 session).
4. Oversampling + GR metering (~2 hr combined).
5. CI workflow.

---

## Project structure

```
stereo-field-compressor/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── Source/
│   ├── PluginProcessor.h
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h
│   ├── PluginEditor.cpp
│   ├── SpatialDetector.h
│   ├── SpatialDetector.cpp
│   ├── ConeWindow.h
│   ├── ConeWindow.cpp
│   ├── ConeCompressor.h
│   └── ConeCompressor.cpp
```
