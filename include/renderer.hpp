#pragma once
#include "model.hpp"
#include <raylib.h>
#include <string>
#include <vector>

// Forward declarations
class ResourceLocation;
class BlockModel;

// Custom camera data structure (to avoid conflicts with raylib's Camera)
struct CustomCamera {
  float posX = 0.0f;
  float posY = -2.0f;
  float posZ = -8.0f;
  float rotX = 15.0f;
  float rotY = 0.0f;
};

// A hybrid renderer class with static core functionality and instance-based
// state
class Renderer {
private:
  // Viewport dimensions
  static int viewportWidth;
  static int viewportHeight;

  // Instance state
  CustomCamera camera;
  bool initialized = false;
  Camera raylibCamera;

public:
  Renderer();
  ~Renderer();

  //-------------------------------------------------------------------------
  // Static methods (can be called without a Renderer instance)
  //-------------------------------------------------------------------------

  // Graphics initialization
  static bool initializeGraphics(int width, int height);
  static void terminateGraphics();
  static void swapBuffers();

  // Texture loading
  static Texture2D loadTexture(const ResourceLocation &location);
  static Texture2D loadTexture(const std::string &texturePath);

  // Raylib state management
  static void setClearColor(float r, float g, float b, float a = 1.0f);
  static void clearBuffers();
  static void setColor(float r, float g, float b, float a = 1.0f);
  static void resetColor();
  static void enableTexturing(bool enable = true);

  // Matrix operations
  static void pushMatrix();
  static void popMatrix();
  static void translate(float x, float y, float z);
  static void rotate(float angle, float x, float y, float z);
  static void scale(float x, float y, float z);

  // Primitive rendering
  static void renderColoredCube(float x, float y, float z, float width,
                                float height, float depth, float r, float g,
                                float b, float a = 1.0f);
  static void renderColoredPlane(float x, float y, float z, float width,
                                 float depth, float r, float g, float b,
                                 float a = 1.0f);
  static void renderPoint(float x, float y, float z, float r, float g, float b,
                          float a = 1.0f);

  //-------------------------------------------------------------------------
  // Instance methods (require a Renderer instance)
  //-------------------------------------------------------------------------

  // Frame management
  void beginFrame();
  void endFrame();

  // Camera controls
  void setCamera(const CustomCamera &cam);
  CustomCamera &getCamera();
  Camera &getRaylibCamera();

  // Model rendering
  void renderModel(const BlockModel &model, float x, float y, float z,
                   float scale_factor = 1.0f);
};
