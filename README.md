# Character Animation Game 🎮

A 2D character animation and rendering game built using **iGraphics** (OpenGL/GLUT wrapper) in C++ with Visual Studio 2013.

## 📋 Features

- Character walking and back animations
- Enemy character sprites with multiple frames
- Background scenes and image rendering
- Sound effects and background music (WAV format)
- Balloon/Bubble pop mechanics
- OpenGL-based rendering pipeline

## 🛠️ Requirements

- **IDE**: Visual Studio 2013 (or later)
- **Graphics Library**: iGraphics (included — `iGraphics.h`)
- **OpenGL**: OpenGL32, GLU32, GLUT32 (included as `.lib` and `.dll`)
- **OS**: Windows (32-bit or 64-bit)

## 📁 Project Structure

```
demo/
├── iMain.cpp              # Main game source file
├── iGraphics.h            # iGraphics library header
├── bitmap_loader.h        # Bitmap image loader
├── stb_image.h            # STB image loading library
├── Balloon.hpp            # Balloon game object
├── Utils.hpp              # Utility functions
├── GLUT32.DLL             # GLUT runtime DLL (required)
├── GLU32.LIB              # GLU library
├── OPENGL32.LIB           # OpenGL library
├── glut32.lib             # GLUT link library
├── glaux.h / Glaux.lib    # GL auxiliary library
├── Images/                # Game image assets
├── *.png / *.jpg          # Sprite and background images
└── *.wav                  # Sound effect and music files
```

## ▶️ How to Run

1. Open `demo.sln` in **Visual Studio 2013**
2. Set build configuration to **Debug** or **Release**
3. Build the solution (`Ctrl + Shift + B`)
4. Run the executable (`F5`)

> **Note:** Make sure `GLUT32.DLL` is in the same directory as the `.exe` file.

## 🎮 Controls

*(Add your game controls here)*

## 📸 Screenshots

*(Add screenshots here)*

## 📄 License

This project is for educational purposes.
