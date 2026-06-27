<div align="center">

# 🎯 FRONT LINE: 1971
A Tactical 2D War Game

![Build](https://img.shields.io/badge/build-passing-brightgreen)
![Platform](https://img.shields.io/badge/platform-Windows-blue)
![Language](https://img.shields.io/badge/language-C%2FC%2B%2B-orange)
![Graphics](https://img.shields.io/badge/graphics-iGraphics%20(OpenGL)-yellow)
![License](https://img.shields.io/badge/license-Educational-lightgrey)

</div>

---

## 📌 Overview

**Front Line: 1971** is a custom-built 2D tactical war engine, written entirely from scratch on top of **iGraphics**, a lightweight OpenGL/GLUT wrapper for C++. What began as a character animation and sprite-rendering prototype has grown into a full top-down combat experience — complete with squad command mechanics, wave-based enemy AI, synchronized audio, and a polished commercial-style UI.

The engine is built around an optimized game loop, a custom bitmap/sprite pipeline, and AABB-based collision detection — designed to stay lightweight and frame-stable even as the number of on-screen units scales up.

## 🚀 Core Gameplay Features

- **Squad-Based Tactical Command** — issue real-time orders (`Move`, `Attack`, `Guard`, `Follow`) to allied AI units
- **Dynamic AI & Wave System** — state-driven enemy behavior with progressive wave spawning, health tracking, and randomized patrol paths
- **Interactive Storytelling** — comic-panel style cutscenes for narrative pacing between levels
- **Commercial-Grade UI/UX** — main menu, settings, dynamic in-game HUD, and level-unlock progression
- **Frame-Stable Asset Streaming** — high-resolution BMP textures and background audio load without dropping frame rate

## 🎨 Animation & Asset Pipeline

The visual and audio foundation that powers every character and effect in the engine:

- Custom `bitmap_loader.h` + `stb_image.h` integration for fast BMP/PNG/JPG sprite and background loading
- Multi-frame character animation states (walk, idle, combat) for fluid movement
- Multi-frame enemy sprite sets for varied, readable AI states
- WAV-based sound effect and background music engine, time-synced to gameplay events
- Lightweight interactive object/physics mechanics (e.g. destructible prop objects) originally prototyped to validate the collision and interaction system before being extended into full combat mechanics

## 🏗️ Tech Stack & Architecture

| Component | Technology |
|---|---|
| Core Logic | C / C++ |
| Graphics Pipeline | iGraphics (OpenGL / GLUT wrapper) |
| IDE / Build Target | Visual Studio 2013+, **x86 (Win32)** |
| Collision Detection | AABB (Axis-Aligned Bounding Box) |
| Image Assets | BMP, PNG, JPG — custom loader + `stb_image.h` |
| Audio | WAV (synchronized SFX & background music) |
| Linked Libraries | `OPENGL32`, `GLU32`, `GLUT32`, `glaux` |

## 🧱 Project Structure

```
FrontLine-1971-TacticalEngine/
├── iMain.cpp              # Core game loop & entry point
├── iGraphics.h            # iGraphics library header
├── bitmap_loader.h        # Custom bitmap/sprite loader
├── stb_image.h            # STB image loading library
├── Utils.hpp              # Shared utility functions
├── GLUT32.DLL             # GLUT runtime (required at build output)
├── GLU32.LIB / OPENGL32.LIB / glut32.lib / Glaux.lib
├── Images/                # Sprite, background & UI assets (.bmp/.png/.jpg)
└── Audio/                 # Sound effects & background music (.wav)
```

## ⚙️ Build & Run

1. Clone the repository:
   ```bash
   git clone https://github.com/SksatoshiKaito/FrontLine-1971-TacticalEngine.git
   ```
2. Open the `.sln` solution file in **Visual Studio 2013** (or later).
3. Set the build configuration target to **x86 (Win32)** — required for iGraphics/OpenGL dependencies.
4. Confirm `iGraphics.h` and the OpenGL `.lib` files are linked under **VC++ Directories**.
5. Make sure `GLUT32.DLL` sits in the same folder as the compiled `.exe`.
6. Press `F5` (Local Windows Debugger) to build and launch.

## 🎮 Controls & Keybinds

| Action | Input |
|---|---|
| **Movement** | `W` `A` `S` `D` or Arrow Keys |
| **Primary Attack** | Left Mouse Click (target coordinates) |
| **Squad: Move** | `M` |
| **Squad: Attack** | `A` |
| **Squad: Follow** | `F` |
| **Squad: Guard** | `G` |
| **Squad: Halt** | `S` |

## 🎬 Gameplay Video

Watch the game in action:

[![Front Line: 1971 Gameplay](https://img.youtube.com/vi/8cp4-e7bVWo/maxresdefault.jpg)](https://youtu.be/8cp4-e7bVWo?si=FTd6e0fBds9R6iGp)

**[▶️ Watch Full Gameplay on YouTube](https://youtu.be/8cp4-e7bVWo?si=FTd6e0fBds9R6iGp)**

## 🛡️ License

This project is provided for educational and portfolio demonstration purposes. All custom code, assets, and storyboarding belong to the repository owner.

---

