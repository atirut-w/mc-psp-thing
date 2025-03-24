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

PSP_MODULE_INFO("GLTest", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);

Camera3D camera = {
    {5.0f, 5.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 45.0f,
    CAMERA_PERSPECTIVE,
};

MCPSP::Model model(MCPSP::ResourceLocation("minecraft:block/redstone_torch"));

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

void drawScene() {
  BeginMode3D(camera);

  // Draw the model
  model.draw({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

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
    DrawFPS(10, 10);
    DrawTextf("Camera Position: (%.2f, %.2f, %.2f)", 10, 30, 20, WHITE,
              camera.position.x, camera.position.y, camera.position.z);

    UpdateCamera(&camera, CAMERA_ORBITAL);
    drawScene();

    EndDrawing();
  }

  return 0;
}
