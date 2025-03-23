#pragma once
#include "resource_location.hpp"
#include <raylib.h>
#include <array>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct UV {
  float x1, y1, x2, y2;

  UV() : x1(0), y1(0), x2(1), y2(1) {}
  UV(float x1, float y1, float x2, float y2) : x1(x1), y1(y1), x2(x2), y2(y2) {}
};

struct ModelFace {
  std::string texture;
  UV uv;
  std::string cullface;
  int rotation = 0;
  int tintindex = -1;

  ModelFace() : cullface("") {}
};

struct ModelRotation {
  std::array<float, 3> origin = {0, 0, 0};
  std::string axis;
  float angle = 0.0f;
  bool rescale = false;
};

struct ModelElement {
  std::array<float, 3> from = {0, 0, 0};
  std::array<float, 3> to = {16, 16, 16};
  std::optional<ModelRotation> rotation;
  bool shade = true;
  int light_emission = 0;
  std::unordered_map<std::string, ModelFace> faces;
};

struct DisplayTransform {
  std::array<float, 3> rotation = {0, 0, 0};
  std::array<float, 3> translation = {0, 0, 0};
  std::array<float, 3> scale = {1, 1, 1};
};

// Forward declaration
class TextureManager;

class BlockModel {
private:
  std::string parent;
  bool ambientocclusion = true;
  std::unordered_map<std::string, DisplayTransform> display;
  std::unordered_map<std::string, std::string> textures;
  std::vector<ModelElement> elements;

  // Helper methods
  void loadParentModel(const std::string &parentPath);
  void parseModelData(const nlohmann::json &data);
  void parseElements(const nlohmann::json &elementsJson);
  void parseTextures(const nlohmann::json &texturesJson);
  void parseDisplay(const nlohmann::json &displayJson);
  ModelFace parseFace(const nlohmann::json &faceJson);
  ModelElement parseElement(const nlohmann::json &elementJson);
  DisplayTransform parseDisplayTransform(const nlohmann::json &transformJson);

  // Rendering helper methods
  void renderElement(const ModelElement &element) const;
  void renderFace(const std::string &faceName, const ModelFace &face, float x1,
                  float y1, float z1, float x2, float y2, float z2) const;

public:
  BlockModel(const ResourceLocation &location);

  // Load the model from a file
  void loadModel(const ResourceLocation &location);

  // Resolve texture references (handles #texture_variable references)
  std::string resolveTexture(const std::string &name);

  // Get all model elements
  const std::vector<ModelElement> &getElements() const { return elements; }

  // Get ambient occlusion setting
  bool hasAmbientOcclusion() const { return ambientocclusion; }

  // Get display transformations
  const std::unordered_map<std::string, DisplayTransform> &getDisplay() const {
    return display;
  }

  // Get particle texture
  std::string getParticleTexture() const;

  // Render the model
  void render() const;
};
