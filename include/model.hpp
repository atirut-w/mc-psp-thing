#pragma once
#include "resource_location.hpp"
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace MCPSP {

struct ModelFace {
  Vector2 uv1;
  Vector2 uv2;
  std::string texture;
  int rotation = 0;
  int tintindex = -1;
};

struct ElementRotation {
  Vector3 origin = {0, 0, 0};
  std::string axis = "x";
  float angle = 0.0f;
};

struct ModelElement {
  Vector3 from;
  Vector3 to;
  ElementRotation rotation;
  std::unordered_map<std::string, ModelFace> faces;
};

class Model {
  std::unordered_map<std::string, std::string> textures;
  std::vector<ModelElement> elements;

  void loadModel(const MCPSP::ResourceLocation &location);

public:
  Model() = default;
  Model(const MCPSP::ResourceLocation &location);

  const std::vector<ModelElement> &getElements() const { return elements; }
  ResourceLocation resolveTexture(const std::string &texture) const;
};

} // namespace MCPSP
