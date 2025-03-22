#include "model.hpp"
#include "texture_manager.hpp"
#include <GLES/egl.h>
#include <GLES/gl.h>
#include <iostream>
#include <math.h>
#include <png.h>
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

// OpenGL ES state variables
EGLDisplay display;
EGLContext context;
EGLSurface surface;

// Camera variables
float cameraX = 0.0f;
float cameraY = -2.0f;     // Start a bit higher to see the blocks better
float cameraZ = -8.0f;    // Start further back to see more of the scene
float cameraRotX = 15.0f; // Start with a slight downward tilt
float cameraRotY = 0.0f;

// Texture manager
TextureManager textureManager;

// Custom function for perspective projection since GLES 1.0 doesn't include
// gluPerspective
void gluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar) {
  GLfloat frustumHeight = tanf(fovy * M_PI / 360.0f) * zNear;
  GLfloat frustumWidth = frustumHeight * aspect;

  glFrustumf(-frustumWidth, frustumWidth, -frustumHeight, frustumHeight, zNear,
             zFar);
}

// PNG texture loading function
GLuint loadPNGTexture(const char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    printf("Error opening file %s\n", filename);
    return 0;
  }

  // Read PNG signature
  png_byte header[8];
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) {
    printf("Not a valid PNG file: %s\n", filename);
    fclose(fp);
    return 0;
  }

  // Create PNG structs
  png_structp png_ptr =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fclose(fp);
    return 0;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    fclose(fp);
    return 0;
  }

  // Set jump for error handling
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return 0;
  }

  // Initialize PNG IO
  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8); // Skip signature

  // Read PNG info
  png_read_info(png_ptr, info_ptr);

  // Get image dimensions and format
  int width = png_get_image_width(png_ptr, info_ptr);
  int height = png_get_image_height(png_ptr, info_ptr);
  png_byte color_type = png_get_color_type(png_ptr, info_ptr);
  png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

  // Make sure PNG is in a format OpenGL can use
  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png_ptr);

  // Convert grayscale to RGB
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png_ptr);

  // Add alpha channel if none exists
  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);

  // Update info after transformations
  png_read_update_info(png_ptr, info_ptr);

  // Read image data
  int rowbytes = png_get_rowbytes(png_ptr, info_ptr);
  png_byte *image_data = (png_byte *)malloc(rowbytes * height);
  png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);

  for (int i = 0; i < height; i++) {
    row_pointers[i] = image_data + i * rowbytes;
  }

  png_read_image(png_ptr, row_pointers);
  fclose(fp);

  // Create OpenGL texture
  GLuint textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Upload texture data to GPU
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, image_data);

  // Clean up
  free(image_data);
  free(row_pointers);
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  return textureID;
}

int initGL() {
  // Setup EGL
  EGLint majorVersion, minorVersion;
  EGLConfig config;
  EGLint numConfigs;

  EGLint attribList[] = {EGL_RED_SIZE,   8,  EGL_GREEN_SIZE, 8,
                         EGL_BLUE_SIZE,  8,  EGL_ALPHA_SIZE, 8,
                         EGL_DEPTH_SIZE, 16, EGL_NONE};

  // Get display
  display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (display == EGL_NO_DISPLAY) {
    return -1;
  }

  // Initialize
  if (!eglInitialize(display, &majorVersion, &minorVersion)) {
    return -1;
  }

  // Get config
  if (!eglChooseConfig(display, attribList, &config, 1, &numConfigs)) {
    return -1;
  }

  // Create surface
  surface = eglCreateWindowSurface(display, config, 0, NULL);
  if (surface == EGL_NO_SURFACE) {
    return -1;
  }

  // Create context
  context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
  if (context == EGL_NO_CONTEXT) {
    return -1;
  }

  // Make current
  if (!eglMakeCurrent(display, surface, surface, context)) {
    return -1;
  }

  // Setup viewport
  glViewport(0, 0, 480, 272);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  // Use perspective projection instead of orthographic
  gluPerspective(45.0f, 480.0f / 272.0f, 0.1f, 100.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Enable depth testing
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  // Disable culling for now to see all faces
  glDisable(GL_CULL_FACE);

  // Initialize resources
  // Create test blocks directly here

  // Enable texturing
  glEnable(GL_TEXTURE_2D);

  // Set the texture manager for the Model class
  Model::setTextureManager(&textureManager);

  return 0;
}

void drawScene() {
  // Reference to the global blockModels vector
  glClearColor(0.0f, 0.5f, 0.8f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLoadIdentity();

  // Set camera position and rotation
  glRotatef(cameraRotX, 1.0f, 0.0f, 0.0f);
  glRotatef(cameraRotY, 0.0f, 1.0f, 0.0f);
  glTranslatef(cameraX, cameraY, cameraZ);

  // Asset Zoo scene
  {
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
    glPushMatrix();

    // Create a 4x4 grid layout for our models (adjust as needed based on model
    // count)
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

          // Draw a platform underneath each model
          glPushMatrix();
          glTranslatef(x, -0.6f, z);
          glScalef(1.5f, 0.1f, 1.5f);

          // Platform color based on row/col for easier identification
          float r = (float)col / gridSize;
          float g = 0.6f;
          float b = (float)row / gridSize;

          glDisable(GL_TEXTURE_2D);
          glColor4f(r, g, b, 1.0f);

          // Draw platform as a simple cube
          static const GLfloat platformVerts[] = {
              // Top face
              -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, -0.5f,
              0.5f, 0.5f,
              // Bottom face
              -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f,
              -0.5f, 0.5f,
              // Front face
              -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f,
              0.5f, 0.5f,
              // Back face
              -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f,
              0.5f, -0.5f,
              // Left face
              -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,
              0.5f, -0.5f,
              // Right face
              0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
              0.5f, -0.5f};

          glEnableClientState(GL_VERTEX_ARRAY);
          glVertexPointer(3, GL_FLOAT, 0, platformVerts);

          // Indices for drawing the platform (6 faces * 2 triangles * 3
          // vertices)
          static const GLubyte platformIndices[] = {
              0,  1,  2,  0,  2,  3,  // Top face
              4,  5,  6,  4,  6,  7,  // Bottom face
              8,  9,  10, 8,  10, 11, // Front face
              12, 13, 14, 12, 14, 15, // Back face
              16, 17, 18, 16, 18, 19, // Left face
              20, 21, 22, 20, 22, 23  // Right face
          };

          glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, platformIndices);
          glDisableClientState(GL_VERTEX_ARRAY);

          glEnable(GL_TEXTURE_2D);
          glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
          glPopMatrix();

          // Now render the actual model
          glPushMatrix();
          glTranslatef(x, 0.0f, z);

          // Render the model
          blockModels[index].second.render();

          // Add a small label (for now just a colored marker)
          glDisable(GL_TEXTURE_2D);
          glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
          glBegin(GL_POINTS);
          glVertex3f(0.0f, 1.2f, 0.0f);
          glEnd();
          glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
          glEnable(GL_TEXTURE_2D);

          glPopMatrix();

          index++;
        }
      }
    }

    // Draw a ground plane
    glPushMatrix();
    glTranslatef(0.0f, -1.0f, 0.0f);
    glScalef(20.0f, 0.1f, 20.0f);

    glDisable(GL_TEXTURE_2D);
    glColor4f(0.2f, 0.2f, 0.3f, 1.0f);

    // Simple ground plane vertices
    static const GLfloat groundVerts[] = {-0.5f, 0.0f,  -0.5f, 0.5f,
                                          0.0f,  -0.5f, 0.5f,  0.0f,
                                          0.5f,  -0.5f, 0.0f,  0.5f};

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, groundVerts);

    GLubyte groundIndices[] = {0, 1, 2, 0, 2, 3};
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, groundIndices);

    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glPopMatrix();

    glPopMatrix();
  }

  glDisableClientState(GL_VERTEX_ARRAY);
  glEnable(GL_TEXTURE_2D);

  eglSwapBuffers(display, surface);
}

int main(int argc, char *argv[]) {
  setupCallbacks();

  // Initialize OpenGL
  if (initGL() < 0) {
    return 1;
  }

  // Variables for FPS calculation
  u64 lastTick = 0;
  u64 currentTick = 0;
  float fps = 0.0f;
  int frameCount = 0;

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

  int running = 1;
  while (running) {
    // Get current time for FPS calculation
    sceRtcGetCurrentTick(&currentTick);
    frameCount++;

    // Calculate and print FPS approximately once per second
    if (lastTick == 0) {
      lastTick = currentTick;
    } else if (currentTick - lastTick >= sceRtcGetTickResolution()) {
      // Calculate FPS and reset counters
      fps = (float)frameCount /
            ((currentTick - lastTick) / (float)sceRtcGetTickResolution());
      printf("FPS: %.1f - Models: %d - Camera: %.1f, %.1f, %.1f\n", fps,
             (int)blockModels.size(), cameraX, cameraY, cameraZ);

      frameCount = 0;
      lastTick = currentTick;
    }

    // Read controller input
    sceCtrlReadBufferPositive(&pad, 1);

    if (pad.Buttons & PSP_CTRL_START) {
      running = 0;
    }

    // Analog stick for movement (forward/backward, left/right)
    if (pad.Lx != 128 || pad.Ly != 128) {
      float dx = (pad.Lx - 128) * ANALOG_SENSITIVITY;
      float dy = (pad.Ly - 128) * ANALOG_SENSITIVITY;

      // Apply movement in the direction the camera is facing
      float angleRad = cameraRotY * M_PI / 180.0f;
      float pitchRad = cameraRotX * M_PI / 180.0f;

      // Horizontal movement (left/right)
      cameraX -= cosf(angleRad) * dx * MOVEMENT_SPEED;
      cameraZ -= sinf(angleRad) * dx * MOVEMENT_SPEED;

      // Forward/backward with vertical component based on camera pitch
      cameraX += sinf(angleRad) * dy * MOVEMENT_SPEED;
      cameraY -=
          sinf(pitchRad) * dy * MOVEMENT_SPEED; // Y movement based on pitch
      cameraZ -= cosf(angleRad) * dy * MOVEMENT_SPEED;
    }

    // Button controls for rotation
    if (pad.Buttons & PSP_CTRL_TRIANGLE) {
      // Look up
      cameraRotX -= ROTATION_SPEED;
      if (cameraRotX < -90.0f)
        cameraRotX = -90.0f;
    }

    if (pad.Buttons & PSP_CTRL_CROSS) {
      // Look down
      cameraRotX += ROTATION_SPEED;
      if (cameraRotX > 90.0f)
        cameraRotX = 90.0f;
    }

    if (pad.Buttons & PSP_CTRL_SQUARE) {
      // Turn left
      cameraRotY -= ROTATION_SPEED;
      if (cameraRotY < 0.0f)
        cameraRotY += 360.0f;
    }

    if (pad.Buttons & PSP_CTRL_CIRCLE) {
      // Turn right
      cameraRotY += ROTATION_SPEED;
      if (cameraRotY > 360.0f)
        cameraRotY -= 360.0f;
    }

    // D-pad for direct vertical movement
    if (pad.Buttons & PSP_CTRL_UP) {
      // Move up
      cameraY -= MOVEMENT_SPEED;
    }

    if (pad.Buttons & PSP_CTRL_DOWN) {
      // Move down
      cameraY += MOVEMENT_SPEED;
    }

    drawScene();
  }

  // Clean up resources
  textureManager.clearTextures();

  // Terminate EGL
  eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroyContext(display, context);
  eglDestroySurface(display, surface);
  eglTerminate(display);

  return 0;
}
