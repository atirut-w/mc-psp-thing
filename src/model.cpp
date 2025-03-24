#include "model.hpp"
#include "raylib.h"
#include "resource_location.hpp"
#include "texture_manager.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
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

      modelElement.from = {element["from"][0].get<float>() / 16.0f,
                           element["from"][1].get<float>() / 16.0f,
                           element["from"][2].get<float>() / 16.0f};
      modelElement.to = {element["to"][0].get<float>() / 16.0f,
                         element["to"][1].get<float>() / 16.0f,
                         element["to"][2].get<float>() / 16.0f};

      if (element.contains("rotation")) {
        const auto &rotation = element["rotation"];
        modelElement.rotation.origin = {rotation["origin"][0],
                                        rotation["origin"][1],
                                        rotation["origin"][2]};
        modelElement.rotation.axis = rotation["axis"];
        modelElement.rotation.angle = rotation["angle"].get<float>() * 22.5f;
        modelElement.rotation.rescale = rotation.value("rescale", false);
      }
      if (element.contains("faces")) {
        for (auto &[key, value] : element["faces"].items()) {
          ModelFace face;

          face.uv.from = {value["uv"][2].get<float>() / 16.0f,
                          value["uv"][1].get<float>() / 16.0f};
          face.uv.to = {value["uv"][0].get<float>() / 16.0f,
                        value["uv"][3].get<float>() / 16.0f};

          face.texture = value["texture"];
          face.cullface = value.value("cullface", "");
          face.rotation = value.value("rotation", 0);
          face.tintindex = value.value("tintindex", false);
          modelElement.faces[key] = face;
        }
      }
      elements.push_back(modelElement);
    }
  }
}

ResourceLocation Model::resolveTexture(const std::string &texture) {
  if (texture.find("#") != std::string::npos) {
    return resolveTexture(textures[texture.substr(1)]);
  }
  return ResourceLocation(texture);
}

void Model::draw(const std::array<float, 3> &position,
                 const std::array<float, 3> &rotation,
                 const std::array<float, 3> &scale) {
  for (const auto &element : elements) {
    for (const auto &[key, face] : element.faces) {
      rlPushMatrix();
      rlTranslatef(position[0], position[1], position[2]);
      rlRotatef(rotation[0], 1.0f, 0.0f, 0.0f);
      rlRotatef(rotation[1], 0.0f, 1.0f, 0.0f);
      rlRotatef(rotation[2], 0.0f, 0.0f, 1.0f);
      rlScalef(scale[0], scale[1], scale[2]);

      // Apply element rotation if present
      if (element.rotation.angle != 0.0f) {
        float ox = element.rotation.origin[0] / 16.0f;
        float oy = element.rotation.origin[1] / 16.0f;
        float oz = element.rotation.origin[2] / 16.0f;

        rlTranslatef(ox, oy, oz);
        if (element.rotation.axis == "x")
          rlRotatef(element.rotation.angle, 1.0f, 0.0f, 0.0f);
        else if (element.rotation.axis == "y")
          rlRotatef(element.rotation.angle, 0.0f, 1.0f, 0.0f);
        else if (element.rotation.axis == "z")
          rlRotatef(element.rotation.angle, 0.0f, 0.0f, 1.0f);
        rlTranslatef(-ox, -oy, -oz);
      }

      rlBegin(RL_QUADS);
      const Texture2D &texture =
          TextureManager::getTexture(resolveTexture(face.texture));
      rlSetTexture(texture.id);
      rlColor4f(1.0f, 1.0f, 1.0f, 1.0f);

      // Calculate UVs based on rotation
      float u0 = face.uv.from[0], v0 = face.uv.from[1];
      float u1 = face.uv.to[0], v1 = face.uv.to[1];

      // Apply face rotation if needed
      if (face.rotation != 0) {
        // Store original UVs
        float u0_orig = u0, v0_orig = v0;
        float u1_orig = u1, v1_orig = v1;

        // Apply rotation (in steps of 90 degrees)
        switch (face.rotation) {
        case 90:
          u0 = u0_orig;
          v0 = v1_orig;
          u1 = u1_orig;
          v1 = v0_orig;
          break;
        case 180:
          u0 = u1_orig;
          v0 = v1_orig;
          u1 = u0_orig;
          v1 = v0_orig;
          break;
        case 270:
          u0 = u1_orig;
          v0 = v0_orig;
          u1 = u0_orig;
          v1 = v1_orig;
          break;
        }
      }

      float minX = element.from[0], minY = element.from[1],
            minZ = element.from[2];
      float maxX = element.to[0], maxY = element.to[1], maxZ = element.to[2];

      if (key == "north") {
        rlTexCoord2f(u0, v0);
        rlVertex3f(minX, maxY, minZ);
        rlTexCoord2f(u1, v0);
        rlVertex3f(maxX, maxY, minZ);
        rlTexCoord2f(u1, v1);
        rlVertex3f(maxX, minY, minZ);
        rlTexCoord2f(u0, v1);
        rlVertex3f(minX, minY, minZ);
      } else if (key == "south") {
        rlTexCoord2f(u0, v0);
        rlVertex3f(maxX, maxY, maxZ);
        rlTexCoord2f(u1, v0);
        rlVertex3f(minX, maxY, maxZ);
        rlTexCoord2f(u1, v1);
        rlVertex3f(minX, minY, maxZ);
        rlTexCoord2f(u0, v1);
        rlVertex3f(maxX, minY, maxZ);
      } else if (key == "west") {
        rlTexCoord2f(u0, v0);
        rlVertex3f(minX, maxY, maxZ);
        rlTexCoord2f(u1, v0);
        rlVertex3f(minX, maxY, minZ);
        rlTexCoord2f(u1, v1);
        rlVertex3f(minX, minY, minZ);
        rlTexCoord2f(u0, v1);
        rlVertex3f(minX, minY, maxZ);
      } else if (key == "east") {
        rlTexCoord2f(u0, v0);
        rlVertex3f(maxX, maxY, minZ);
        rlTexCoord2f(u1, v0);
        rlVertex3f(maxX, maxY, maxZ);
        rlTexCoord2f(u1, v1);
        rlVertex3f(maxX, minY, maxZ);
        rlTexCoord2f(u0, v1);
        rlVertex3f(maxX, minY, minZ);
      } else if (key == "up") {
        rlTexCoord2f(u0, v0);
        rlVertex3f(minX, maxY, maxZ);
        rlTexCoord2f(u1, v0);
        rlVertex3f(maxX, maxY, maxZ);
        rlTexCoord2f(u1, v1);
        rlVertex3f(maxX, maxY, minZ);
        rlTexCoord2f(u0, v1);
        rlVertex3f(minX, maxY, minZ);
      } else if (key == "down") {
        rlTexCoord2f(u0, v0);
        rlVertex3f(minX, minY, minZ);
        rlTexCoord2f(u1, v0);
        rlVertex3f(maxX, minY, minZ);
        rlTexCoord2f(u1, v1);
        rlVertex3f(maxX, minY, maxZ);
        rlTexCoord2f(u0, v1);
        rlVertex3f(minX, minY, maxZ);
      }

      rlEnd();
      rlSetTexture(0);
      rlPopMatrix();
    }
  }
}

} // namespace MCPSP
