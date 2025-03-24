#include "model.hpp"
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psprtc.h>
#include <pspthreadman.h>
#include <psptypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>

PSP_MODULE_INFO("GLTest", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);

Camera3D camera = {
  {5.0f, 5.0f, 5.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 1.0f, 0.0f},
  45.0f,
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

void drawScene() {
  BeginMode3D(camera);

  // Draw a cube
  DrawCube((Vector3){0.0f, 1.0f, 0.0f}, 2.0f, 2.0f, 2.0f, RED);
  DrawCubeWires((Vector3){0.0f, 1.0f, 0.0f}, 2.0f, 2.0f, 2.0f, MAROON);

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

    UpdateCamera(&camera, CAMERA_ORBITAL);
    drawScene();

    EndDrawing();
  }

  return 0;
}
