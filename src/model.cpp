#include "model.hpp"
#include "raylib.h"
#include "resource_location.hpp"
#include "texture_manager.hpp"
#include <GL/gl.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <raymath.h>
#include <rlgl.h>

namespace MCPSP {

Model::Model(const MCPSP::ResourceLocation &location) { loadModel(location); }

void Model::loadModel(const MCPSP::ResourceLocation &location) {
  std::string path = location.resolvePath("models") + ".json";

  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open model file: " + path);
  }

  nlohmann::json json;
  try {
    file >> json;
  } catch (const nlohmann::json::parse_error &e) {
    throw std::runtime_error("failed to parse model file: " + path);
  }
  file.close();

  std::cout << "Loading model from: " << path << std::endl;

  if (json.contains("parent")) {
    loadModel(ResourceLocation(json["parent"]));
  }
  if (json.contains("textures")) {
    for (auto &[key, value] : json["textures"].items()) {
      textures[key] = value;
    }
  }
  if (json.contains("elements")) {
    for (const auto &element : json["elements"]) {
      ModelElement modelElement;

      modelElement.from = {element["from"][0], element["from"][1],
                           element["from"][2]};
      modelElement.to = {element["to"][0], element["to"][1], element["to"][2]};

      modelElement.from.x /= 16.0f;
      modelElement.from.y /= 16.0f;
      modelElement.from.z /= 16.0f;
      modelElement.to.x /= 16.0f;
      modelElement.to.y /= 16.0f;
      modelElement.to.z /= 16.0f;

      if (element.contains("rotation")) {
        modelElement.rotation.origin = {element["rotation"]["origin"][0],
                                        element["rotation"]["origin"][1],
                                        element["rotation"]["origin"][2]};

        modelElement.rotation.origin.x /= 16.0f;
        modelElement.rotation.origin.y /= 16.0f;
        modelElement.rotation.origin.z /= 16.0f;

        modelElement.rotation.axis = element["rotation"]["axis"];
        modelElement.rotation.angle = element["rotation"]["angle"];
      }

      if (element.contains("faces")) {
        for (const auto &[direction, face] : element["faces"].items()) {
          ModelFace modelFace;

          Vector2 uv1;
          Vector2 uv2;
          if (face.contains("uv")) {
            uv1 = {face["uv"][0], face["uv"][1]};
            uv2 = {face["uv"][2], face["uv"][3]};

            uv1.x /= 16.0f;
            uv1.y /= 16.0f;
            uv2.x /= 16.0f;
            uv2.y /= 16.0f;
          } else {
            // Generate UVs based on model coordinates
            // Note: from/to are already normalized to 0-1 range (divided by 16)
            if (direction == "north") {
              uv1 = {modelElement.to.x, 1.0f - modelElement.to.y}; // Top-right
              uv2 = {modelElement.from.x,
                     1.0f - modelElement.from.y}; // Bottom-left
            } else if (direction == "south") {
              uv1 = {modelElement.from.x, 1.0f - modelElement.to.y}; // Top-left
              uv2 = {modelElement.to.x,
                     1.0f - modelElement.from.y}; // Bottom-right
            } else if (direction == "west") {
              uv1 = {modelElement.to.z, 1.0f - modelElement.to.y}; // Top-right
              uv2 = {modelElement.from.z,
                     1.0f - modelElement.from.y}; // Bottom-left
            } else if (direction == "east") {
              uv1 = {modelElement.from.z, 1.0f - modelElement.to.y}; // Top-left
              uv2 = {modelElement.to.z,
                     1.0f - modelElement.from.y}; // Bottom-right
            } else if (direction == "up") {
              uv1 = {modelElement.from.x, modelElement.from.z}; // Top-left
              uv2 = {modelElement.to.x, modelElement.to.z};     // Bottom-right
            } else if (direction == "down") {
              uv1 = {modelElement.from.x, modelElement.to.z}; // Top-left
              uv2 = {modelElement.to.x, modelElement.from.z}; // Bottom-right
            }
          }
          std::swap(uv1.x, uv2.x);

          modelFace.uv1 = uv1;
          modelFace.uv2 = uv2;

          if (face.contains("texture")) {
            modelFace.texture = face["texture"];
          }

          if (face.contains("cullface")) {
            modelFace.cullface = face["cullface"];
          }

          if (face.contains("rotation")) {
            modelFace.rotation = face["rotation"];
          }

          if (face.contains("tintindex")) {
            modelFace.tintindex = face["tintindex"];
          }

          modelElement.faces[direction] = modelFace;
        }

        elements.push_back(modelElement);
      }
    }
  }
}

ResourceLocation Model::resolveTexture(const std::string &texture) const {
  if (texture.find("#") != std::string::npos) {
    return resolveTexture(textures.at(texture.substr(1)));
  }
  return ResourceLocation(texture);
}

} // namespace MCPSP
