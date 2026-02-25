# Orbinal — Real‑Time Satellite Telemetry HUD

<img src="/home/kael/projects/hobby/ui-stuff/orbinal/image.png" alt="Orbinal Screenshot">

**Orbinal** is a Qt‑based, GPU‑accelerated satellite tracking and telemetry visualization system.  
It displays live orbital positions, signal waveforms, radar sweeps, and formatted telemetry data in a retro‑HUD style.

---

## Features

- Eats your gpu

---

## Controls

- **Mouse** – Globe auto‑rotates; no manual interaction needed
- **Close** – Use the custom title bar close button
- **THERES NO CONTROL** - ...

---

## Build Requirements

- **Qt 5/6** (Core, Widgets, OpenGL, OpenGLWidgets) (I used qt5 btw)
- **OpenGL 3.2+**
- **CUDA Toolkit 13.1+** (optional; CPU fallback if missing)
- **CMake 3.18+**
- **GCC 15+** (or compatible C++17 compiler)

---

## Build & Run

```bash
# Clone and configure
git clone https://github.com/kaelvalen/orbinal.git
cd orbinal
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# Run
./orbinal
```

---

## Architecture

- `GlobeWidget` – OpenGL 3D rendering, solid sphere shader, ground tracks
- `SignalGraphWidget` – Waveform + FFT spectrum visualization
- `GeoSyncPanel` – Telemetry readouts, UTC timestamps, live orbit phase diagram
- `RadarWidget` – Embedded radar sweep with GPU blip detection
- `CudaCompute` – Bridge to CUDA kernels (orbit, signal, radar)
- `OrbitalKernels.cu` – GPU kernels for parallel computation
- `CrtOverlay` – Full‑screen post‑process (scanlines, vignette, alerts)

---

## License

MIT License — see LICENSE file for details.
