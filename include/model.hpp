#pragma once
#include "resource_location.hpp"
#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace MCPSP {

struct ModelRotation {
  std::array<float, 3> origin = {0.0f, 0.0f, 0.0f};
  std::string axis = "x";
  float angle = 0.0f;
  bool rescale = false;
};

struct ModelUV {
  std::array<float, 2> from;
  std::array<float, 2> to;
};

struct ModelFace {
  ModelUV uv;
  std::string texture;
  std::string cullface;
  int rotation = 0;
  bool tintindex = false;
};

struct ModelElement {
  std::array<float, 3> from;
  std::array<float, 3> to;
  ModelRotation rotation;
  std::unordered_map<std::string, ModelFace> faces;
};

class Model {
  std::unordered_map<std::string, std::string> textures;
  std::vector<ModelElement> elements;

  void loadModel(const MCPSP::ResourceLocation &location);

public:
  Model(const MCPSP::ResourceLocation &location);
};

} // namespace MCPSP
