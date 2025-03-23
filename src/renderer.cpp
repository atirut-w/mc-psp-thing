#include "renderer.hpp"
#include "texture_manager.hpp"
#include <iostream>
#include <math.h>
#include <rlgl.h>

// Initialize static members
int Renderer::viewportWidth = 480;
int Renderer::viewportHeight = 272;

Renderer::Renderer() {
  // Initialize default values
  initialized = false;
  
  // Initialize raylib camera
  raylibCamera.position = (Vector3){ 0.0f, 2.0f, 8.0f };  // Camera position
  raylibCamera.target = (Vector3){ 0.0f, 0.0f, 0.0f };    // Camera looking at point
  raylibCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };        // Camera up vector
  raylibCamera.fovy = 45.0f;                              // Camera field-of-view Y
  raylibCamera.projection = CAMERA_PERSPECTIVE;           // Camera mode type
}

Renderer::~Renderer() {
  // Instance destruction doesn't terminate graphics
  // That must be done explicitly with terminateGraphics()
}

bool Renderer::initializeGraphics(int width, int height) {
  // Save viewport dimensions
  viewportWidth = width;
  viewportHeight = height;

  // Initialize raylib window
  InitWindow(width, height, "GLTest");
  
  // Set target FPS (maximum)
  SetTargetFPS(60);
  
  // Enable depth testing
  return IsWindowReady();
}

void Renderer::terminateGraphics() {
  // Close raylib window and resources
  CloseWindow();
}

void Renderer::swapBuffers() {
  // No need to manually swap buffers in raylib, EndDrawing() handles it
}

Texture2D Renderer::loadTexture(const ResourceLocation &location) {
  // Delegate to the TextureManager's static method
  return TextureManager::loadTexture(location);
}

Texture2D Renderer::loadTexture(const std::string &texturePath) {
  // Delegate to the TextureManager's static method
  return TextureManager::getTexture(texturePath);
}

void Renderer::setClearColor(float r, float g, float b, float a) {
  ClearBackground((Color){ (unsigned char)(r*255), (unsigned char)(g*255), 
                          (unsigned char)(b*255), (unsigned char)(a*255) });
}

void Renderer::clearBuffers() {
  // Clearing is handled in BeginDrawing() in raylib
}

void Renderer::beginFrame() {
  BeginDrawing();
  ClearBackground((Color){ 0, 127, 204, 255 }); // Sky-blue background
  
  BeginMode3D(raylibCamera);
  
  // Update camera from our custom camera
  raylibCamera.position.x = -camera.posX;
  raylibCamera.position.y = -camera.posY;
  raylibCamera.position.z = -camera.posZ;
  
  // Use the camera rotation to update the target point
  float angleX = camera.rotX * DEG2RAD;
  float angleY = camera.rotY * DEG2RAD;
  
  // Calculate target point based on rotation angles
  raylibCamera.target.x = raylibCamera.position.x + sin(angleY);
  raylibCamera.target.y = raylibCamera.position.y - sin(angleX);
  raylibCamera.target.z = raylibCamera.position.z - cos(angleY);
}

void Renderer::endFrame() {
  EndMode3D();
  EndDrawing();
}

void Renderer::setCamera(const CustomCamera &cam) { 
  camera = cam; 
}

CustomCamera &Renderer::getCamera() { 
  return camera; 
}

Camera &Renderer::getRaylibCamera() {
  return raylibCamera;
}

void Renderer::renderModel(const BlockModel &model, float x, float y, float z,
                           float scale_factor) {
  pushMatrix();
  translate(x, y, z);

  if (scale_factor != 1.0f) {
    scale(scale_factor, scale_factor, scale_factor);
  }

  model.render();

  popMatrix();
}

void Renderer::renderColoredCube(float x, float y, float z, float width,
                                 float height, float depth, float r, float g,
                                 float b, float a) {
  Color color = { (unsigned char)(r*255), (unsigned char)(g*255), 
                 (unsigned char)(b*255), (unsigned char)(a*255) };
                 
  // Draw a colored cube
  DrawCube((Vector3){x, y, z}, width, height, depth, color);
}

void Renderer::renderColoredPlane(float x, float y, float z, float width,
                                  float depth, float r, float g, float b,
                                  float a) {
  Color color = { (unsigned char)(r*255), (unsigned char)(g*255), 
                 (unsigned char)(b*255), (unsigned char)(a*255) };
                 
  // Draw a colored plane
  DrawPlane((Vector3){x, y, z}, (Vector2){width, depth}, color);
}

void Renderer::renderPoint(float x, float y, float z, float r, float g, float b,
                           float a) {
  Color color = { (unsigned char)(r*255), (unsigned char)(g*255), 
                 (unsigned char)(b*255), (unsigned char)(a*255) };
                 
  // Draw a 3D point (small sphere for visibility)
  DrawSphere((Vector3){x, y, z}, 0.1f, color);
}

// Matrix operations and state management
// These functions are mainly for compatibility with existing code
void Renderer::setColor(float r, float g, float b, float a) {
  // Raylib uses immediate colors when drawing, stored in individual draw calls
}

void Renderer::resetColor() {
  // Not needed in raylib
}

void Renderer::enableTexturing(bool enable) {
  // Not directly needed in raylib, texture state is per-draw call
}

void Renderer::pushMatrix() {
  rlPushMatrix();
}

void Renderer::popMatrix() {
  rlPopMatrix();
}

void Renderer::translate(float x, float y, float z) {
  rlTranslatef(x, y, z);
}

void Renderer::rotate(float angle, float x, float y, float z) {
  // In raylib we need to rotate around each axis separately if needed
  if (x != 0.0f) rlRotatef(angle, 1.0f, 0.0f, 0.0f);
  if (y != 0.0f) rlRotatef(angle, 0.0f, 1.0f, 0.0f);
  if (z != 0.0f) rlRotatef(angle, 0.0f, 0.0f, 1.0f);
}

void Renderer::scale(float x, float y, float z) {
  rlScalef(x, y, z);
}
