#include "model.hpp"
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psprtc.h>
#include <pspthreadman.h>
#include <psptypes.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>

PSP_MODULE_INFO("GLTest", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);

Camera3D camera = {
    {5.0f, 5.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 45.0f,
    CAMERA_PERSPECTIVE,
};

std::vector<MCPSP::Model> models = {
  MCPSP::Model(MCPSP::ResourceLocation("minecraft:block/potted_acacia_sapling")),
  MCPSP::Model(MCPSP::ResourceLocation("minecraft:block/potted_bamboo")),
  MCPSP::Model(MCPSP::ResourceLocation("minecraft:block/potted_birch_sapling")),
  MCPSP::Model(MCPSP::ResourceLocation("minecraft:block/potted_blue_orchid")),
  MCPSP::Model(MCPSP::ResourceLocation("minecraft:block/potted_cactus")),
  MCPSP::Model(MCPSP::ResourceLocation("minecraft:block/potted_cornflower")),
  MCPSP::Model(MCPSP::ResourceLocation("minecraft:block/potted_dandelion")),
  MCPSP::Model(MCPSP::ResourceLocation("minecraft:block/potted_dark_oak_sapling")),
  MCPSP::Model(MCPSP::ResourceLocation("minecraft:block/potted_fern")),
};

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

void DrawTextf(const char *text, int posX, int posY, int fontSize, Color color,
               ...) {
  va_list args;
  va_start(args, color);
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), text, args);
  va_end(args);
  DrawText(buffer, posX, posY, fontSize, color);
}

void drawModels() {
  int gridSize = (int)sqrt(models.size()) + (sqrt(models.size()) == (int)sqrt(models.size()) ? 0 : 1);
  
  // Calculate the starting position to center the grid
  float startX = -((gridSize - 1) * 2.0f) / 2.0f;
  float startZ = -((gridSize - 1) * 2.0f) / 2.0f;
  
  for (int i = 0; i < models.size(); i++) {
    int row = i / gridSize;
    int col = i % gridSize;
    
    float x = startX + col * 2.0f;
    float z = startZ + row * 2.0f;
    
    models[i].draw({x, 0.0f, z}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
  }
}

void drawScene() {
  BeginMode3D(camera);

  drawModels();

  // Draw a grid
  DrawGrid(10, 1.0f);

  EndMode3D();
}

int main(int argc, char *argv[]) {
  setupCallbacks();

  InitWindow(480, 272, "Minecraft PSP Thing");

  // Main game loop
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground({75, 172, 255});

    drawScene();

    DrawFPS(10, 10);
    DrawTextf("Camera Position: (%.2f, %.2f, %.2f)", 10, 30, 20, WHITE,
              camera.position.x, camera.position.y, camera.position.z);

    UpdateCamera(&camera, CAMERA_ORBITAL);

    EndDrawing();
  }

  return 0;
}
