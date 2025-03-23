#pragma once
#include "resource_location.hpp"
#include <raylib.h>
#include <string>
#include <unordered_map>

class TextureManager {
private:
  static std::unordered_map<std::string, Texture2D> textureCache;

public:
  // Load a texture from a resource location and return its Texture2D
  static Texture2D loadTexture(const ResourceLocation &location);

  // Get a texture by resource location
  static Texture2D getTexture(const ResourceLocation &location);

  // Get a texture by string path
  static Texture2D getTexture(const std::string &texturePath);

  // Check if a texture is already loaded
  static bool isTextureLoaded(const std::string &texturePath);

  // Clear all textures from memory
  static void clearTextures();
};
