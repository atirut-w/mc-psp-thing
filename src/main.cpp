#include "font.hpp"
#include "model.hpp"
#include "renderer.hpp"
#include "texture_manager.hpp"
#include <GLES/egl.h>
#include <GLES/gl.h>
#include <iostream>
#include <math.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psprtc.h>
#include <pspthreadman.h>
#include <psptypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PSP_MODULE_INFO("GLTest", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);

// Global variable for models
std::vector<std::pair<std::string, Model>> blockModels = {
    {"dirt", Model(ResourceLocation("minecraft:block/dirt"))},
    {"stone", Model(ResourceLocation("minecraft:block/stone"))},
    {"oak_log", Model(ResourceLocation("minecraft:block/oak_log"))},
    {"oak_stairs", Model(ResourceLocation("minecraft:block/oak_stairs"))},
    {"oak_door", Model(ResourceLocation("minecraft:block/oak_door_bottom"))},
    {"furnace", Model(ResourceLocation("minecraft:block/furnace"))},
    {"crafting_table",
     Model(ResourceLocation("minecraft:block/crafting_table"))},
    {"chest", Model(ResourceLocation("minecraft:block/chest"))},
    {"bookshelf", Model(ResourceLocation("minecraft:block/bookshelf"))},
    {"glass", Model(ResourceLocation("minecraft:block/glass"))},
    {"grass_block", Model(ResourceLocation("minecraft:block/grass_block"))},
    {"redstone_torch",
     Model(ResourceLocation("minecraft:block/redstone_torch"))},
    {"rail", Model(ResourceLocation("minecraft:block/rail"))},
    {"iron_bars", Model(ResourceLocation("minecraft:block/iron_bars"))},
    {"bed", Model(ResourceLocation("minecraft:block/red_bed_head"))}};

int exitCallback(int arg1, int arg2, void *common) {
  sceKernelExitGame();
  return 0;
}

int callbackThread(SceSize args, void *argp) {
  int cbid = sceKernelCreateCallback("Exit Callback", exitCallback, NULL);
  sceKernelRegisterExitCallback(cbid);
  sceKernelSleepThreadCB();
  return 0;
}

int setupCallbacks(void) {
  int thid =
      sceKernelCreateThread("update_thread", callbackThread, 0x11, 0xFA0, 0, 0);
  if (thid >= 0) {
    sceKernelStartThread(thid, 0, 0);
  }
  return thid;
}

// Renderer instance
Renderer renderer;

// Font instance
Font defaultFont(ResourceLocation("minecraft:default"));

int initGL() {
  // Initialize graphics using the renderer
  if (!Renderer::initializeGraphics(480, 272)) {
    return -1;
  }

  return 0;
}

void drawScene() {
  // Set the sky-blue clear color
  Renderer::setClearColor(0.0f, 0.5f, 0.8f, 1.0f);

  // Begin frame (clears buffers and sets up camera)
  renderer.beginFrame();

  // Debug info for rendering
  static bool first = true;
  if (first) {
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Starting Asset Zoo with " << blockModels.size() << " models"
              << std::endl;
    for (const auto &model : blockModels) {
      std::cout << "- Loaded model: " << model.first << std::endl;
    }
    first = false;
  }

  // Set up model view matrix for rendering blocks
  Renderer::pushMatrix();

  // Create a 4x4 grid layout for our models
  const int gridSize = 4;
  const float spacing = 2.5f; // Spacing between models

  // Center the grid
  float offsetX = -(gridSize * spacing) / 2 + spacing / 2;
  float offsetZ = -(gridSize * spacing) / 2 + spacing / 2;

  // Render each model in its grid position
  int index = 0;
  for (int row = 0; row < gridSize; row++) {
    for (int col = 0; col < gridSize; col++) {
      if (index < blockModels.size()) {
        // Position for this model
        float x = offsetX + col * spacing;
        float z = offsetZ + row * spacing;

        // Platform color based on row/col for easier identification
        float r = (float)col / gridSize;
        float g = 0.6f;
        float b = (float)row / gridSize;

        // Draw a platform underneath each model
        Renderer::renderColoredCube(x, -0.6f, z, 1.5f, 0.1f, 1.5f, r, g, b,
                                    1.0f);

        // Now render the actual model using our renderer
        renderer.renderModel(blockModels[index].second, x, 0.0f, z);

        // Add a small label marker
        Renderer::renderPoint(x, 1.2f, z, 1.0f, 1.0f, 0.0f);

        index++;
      }
    }
  }

  // Draw a ground plane
  Renderer::renderColoredPlane(0.0f, -1.0f, 0.0f, 20.0f, 20.0f, 0.2f, 0.2f,
                               0.3f);

  Renderer::popMatrix();

  // End frame and swap buffers
  renderer.endFrame();
}

int main(int argc, char *argv[]) {
  setupCallbacks();

  // Initialize OpenGL
  if (initGL() < 0) {
    return 1;
  }

  // Main loop variables

  SceCtrlData pad;

  // Enable analog stick
  sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

  // Camera movement constants
  const float ANALOG_SENSITIVITY = 0.005f;
  const float ROTATION_SPEED = 2.0f;  // Faster rotation for better viewing
  const float MOVEMENT_SPEED = 0.08f; // Faster movement to navigate the scene

  // Display help text in console for camera controls
  std::cout << "\n=== Camera Controls ===\n"
            << "Analog Stick: Move forward/backward and strafe left/right\n"
            << "Triangle/Cross: Look up/down\n"
            << "Square/Circle: Turn left/right\n"
            << "D-Pad Up/Down: Move up/down\n"
            << "START: Exit\n"
            << "=====================\n"
            << std::endl;

  // Print font loading information
  std::cout << "\n=== Font Information ===\n"
            << "Default font loaded with " << defaultFont.providers.size()
            << " providers\n";
  int providerIndex = 0;
  for (const auto &provider : defaultFont.providers) {
    std::cout << "Provider " << ++providerIndex << ": ";
    if (dynamic_cast<BitmapFontProvider *>(provider.get())) {
      auto *bitmap = dynamic_cast<BitmapFontProvider *>(provider.get());
      std::cout << "Bitmap provider with file: " << bitmap->file.resolvePath("texture")
                << "\n";
    } else if (dynamic_cast<SpaceFontProvider *>(provider.get())) {
      auto *space = dynamic_cast<SpaceFontProvider *>(provider.get());
      std::cout << "Space provider with " << space->advances.size()
                << " advances\n";
    } else {
      std::cout << "Unknown provider type\n";
    }
  }
  std::cout << "=====================\n" << std::endl;

  int running = 1;
  while (running) {

    // Read controller input
    sceCtrlReadBufferPositive(&pad, 1);

    if (pad.Buttons & PSP_CTRL_START) {
      running = 0;
    }

    // Get camera for manipulation
    Camera &camera = renderer.getCamera();

    // Analog stick for movement (forward/backward, left/right)
    if (pad.Lx != 128 || pad.Ly != 128) {
      float dx = (pad.Lx - 128) * ANALOG_SENSITIVITY;
      float dy = (pad.Ly - 128) * ANALOG_SENSITIVITY;

      // Apply movement in the direction the camera is facing
      float angleRad = camera.rotY * M_PI / 180.0f;
      float pitchRad = camera.rotX * M_PI / 180.0f;

      // Horizontal movement (left/right)
      camera.posX -= cosf(angleRad) * dx * MOVEMENT_SPEED;
      camera.posZ -= sinf(angleRad) * dx * MOVEMENT_SPEED;

      // Forward/backward with vertical component based on camera pitch
      camera.posX += sinf(angleRad) * dy * MOVEMENT_SPEED;
      camera.posY -=
          sinf(pitchRad) * dy * MOVEMENT_SPEED; // Y movement based on pitch
      camera.posZ -= cosf(angleRad) * dy * MOVEMENT_SPEED;
    }

    // Button controls for rotation
    if (pad.Buttons & PSP_CTRL_TRIANGLE) {
      // Look up
      camera.rotX -= ROTATION_SPEED;
      if (camera.rotX < -90.0f)
        camera.rotX = -90.0f;
    }

    if (pad.Buttons & PSP_CTRL_CROSS) {
      // Look down
      camera.rotX += ROTATION_SPEED;
      if (camera.rotX > 90.0f)
        camera.rotX = 90.0f;
    }

    if (pad.Buttons & PSP_CTRL_SQUARE) {
      // Turn left
      camera.rotY -= ROTATION_SPEED;
      if (camera.rotY < 0.0f)
        camera.rotY += 360.0f;
    }

    if (pad.Buttons & PSP_CTRL_CIRCLE) {
      // Turn right
      camera.rotY += ROTATION_SPEED;
      if (camera.rotY > 360.0f)
        camera.rotY -= 360.0f;
    }

    // D-pad for direct vertical movement
    if (pad.Buttons & PSP_CTRL_UP) {
      // Move up
      camera.posY -= MOVEMENT_SPEED;
    }

    if (pad.Buttons & PSP_CTRL_DOWN) {
      // Move down
      camera.posY += MOVEMENT_SPEED;
    }

    drawScene();
  }

  // Clean up resources
  TextureManager::clearTextures();

  // Terminate graphics
  Renderer::terminateGraphics();

  return 0;
}
