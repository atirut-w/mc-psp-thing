#include "texture_manager.hpp"
#include <iostream>
#include <vector>

std::unordered_map<std::string, Texture2D> TextureManager::textureCache;

Texture2D TextureManager::loadTexture(const ResourceLocation &location) {
  std::string texturePath =
      "umd0:/" + location.resolvePath("textures") + ".png";

  // Check if texture is already loaded
  if (isTextureLoaded(texturePath)) {
    return textureCache[texturePath];
  }

  std::cout << "Loading texture from: " << texturePath << std::endl;

  // Use raylib to load the texture
  Texture2D texture = LoadTexture(texturePath.c_str());
  
  if (texture.id == 0) {
    std::cerr << "Error loading texture file: " << texturePath << std::endl;
    return { 0 }; // Return empty texture
  }

  // Set texture filtering (NEAREST for pixel art look)
  SetTextureFilter(texture, TEXTURE_FILTER_POINT);
  SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);

  // Store the complete texture in our cache
  textureCache[texturePath] = texture;

  return texture;
}

Texture2D TextureManager::getTexture(const ResourceLocation &location) {
  std::string texturePath =
      "umd0:/" + location.resolvePath("textures") + ".png";
  return getTexture(texturePath);
}

Texture2D TextureManager::getTexture(const std::string &texturePath) {
  // Direct cache lookup for full paths
  if (isTextureLoaded(texturePath)) {
    return textureCache[texturePath];
  }

  // If not a full path (doesn't start with umd0:), try as a ResourceLocation
  if (texturePath.find("umd0:") == std::string::npos) {
    // ResourceLocation constructor will handle default "minecraft" namespace if
    // needed
    return loadTexture(ResourceLocation(texturePath));
  }

  std::cerr << "Texture not found in cache: " << texturePath << std::endl;
  return { 0 }; // Return empty texture
}

bool TextureManager::isTextureLoaded(const std::string &texturePath) {
  return textureCache.find(texturePath) != textureCache.end();
}

void TextureManager::clearTextures() {
  if (!textureCache.empty()) {
    for (const auto &pair : textureCache) {
      UnloadTexture(pair.second);
    }
    textureCache.clear();
  }
}
