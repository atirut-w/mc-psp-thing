#pragma once
#include "resource_location.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <raylib.h>

namespace MCPSP {

struct Mesh {
  std::vector<Vector3> vertices;
  std::vector<Vector2> uvs;
};

class Model {
  std::unordered_map<std::string, std::string> textures;
  std::unordered_map<std::string, Mesh> meshes;

  void loadModel(const MCPSP::ResourceLocation &location);
  ResourceLocation resolveTexture(const std::string &texture);

public:
  Model(const MCPSP::ResourceLocation &location);

  void draw(const Vector3 &position,
            const Vector3 &rotation,
            const Vector3 &scale);
};

} // namespace MCPSP
