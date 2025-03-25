#pragma once

#include "raylib.h"
#include "resource_location.hpp"
#include <string>
#include <unordered_map>
namespace MCPSP {

class TextureManager {
  static std::unordered_map<std::string, Texture2D> textureCache;

public:
  static const Texture2D &getTexture(const ResourceLocation &location) {
    std::string path = location.resolvePath("textures") + ".png";
    if (textureCache.find(path) == textureCache.end()) {
      Texture2D texture = LoadTexture(path.c_str());
      textureCache[path] = texture;
    }
    return textureCache[path];
  }
};

} // namespace MCPSP
