#include "model.hpp"
#include "resource_location.hpp"
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

          face.uv.from = {value["uv"][0].get<float>() / 16.0f,
                          value["uv"][1].get<float>() / 16.0f};
          face.uv.to = {value["uv"][2].get<float>() / 16.0f,
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

} // namespace MCPSP
