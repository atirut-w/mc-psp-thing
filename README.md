# Minecraft PSP thing
This currently only load models from Minecraft JE into an "asset zoo" to showcase parsing of the models. WIP.

# Build Prerequisites
- [PSPDev](https://github.com/pspdev/pspdev)
- All of Minecraft's resources (can be obtained using [Minecraft Resource Extractor](https://github.com/atirut-w/minecraft-resource-extractor))
- Raylib for the PSP. You can pick the [Raylib4Consoles](https://github.com/raylib4Consoles/raylib) fork or [my fork](https://github.com/atirut-w/raylib) which has z-buffer and controller support.

# Build Instructions
1. Do the usual CMake stuff, except replace `cmake` with `psp-cmake`.
2. After building the project, put the `assets` folder from your extracted Minecraft resources into the same folder as the executable.
3. Run the ELF using PPSSPP.

# PSP Compatibility
Idk. Can't be bothered to implement building an EBOOT.PBP file.
