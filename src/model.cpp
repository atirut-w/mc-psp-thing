#include "model.hpp"
#include "raylib.h"
#include "resource_location.hpp"
#include "texture_manager.hpp"
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
      Vector3 from = {element["from"][0], element["from"][1],
                      element["from"][2]};
      Vector3 to = {element["to"][0], element["to"][1], element["to"][2]};

      from.x /= 16.0f;
      from.y /= 16.0f;
      from.z /= 16.0f;
      to.x /= 16.0f;
      to.y /= 16.0f;
      to.z /= 16.0f;

      Matrix transform = MatrixIdentity();
      if (element.contains("rotation")) {
        const auto &rotation = element["rotation"];
        Vector3 origin = {rotation["origin"][0], rotation["origin"][1],
                          rotation["origin"][2]};
        const std::string axis = rotation["axis"];
        const float angle = rotation["angle"];
        const bool rescale = rotation["rescale"];

        origin.x /= 16.0f;
        origin.y /= 16.0f;
        origin.z /= 16.0f;

        if (rescale) {
          // TODO: Handle rescale
          std::cerr << "Rescale not supported yet" << std::endl;
        }

        transform = MatrixTranslate(origin.x, origin.y, origin.z) * transform;
        if (axis == "x") {
          transform = MatrixRotateX(angle * DEG2RAD) * transform;
        } else if (axis == "y") {
          transform = MatrixRotateY(angle * DEG2RAD) * transform;
        } else if (axis == "z") {
          transform = MatrixRotateZ(angle * DEG2RAD) * transform;
        } else {
          std::cerr << "Unknown rotation axis: " << axis << std::endl;
        }
        transform = MatrixTranslate(-origin.x, -origin.y, -origin.z) * transform;
      }

      if (element.contains("faces")) {
        for (const auto &[direction, face] : element["faces"].items()) {
          if (meshes.find(face["texture"]) == meshes.end()) {
            meshes[face["texture"]] = Mesh();
          }
          Mesh &mesh = meshes[face["texture"]];

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
            uv1 = {0.0f, 0.0f};
            uv2 = {1.0f, 1.0f};
          }
          std::swap(uv1.x, uv2.x);

          if (face.contains("rotation")) {
            int rotation = face["rotation"];
            if (rotation == 90) {
              // Rotate UV coordinates 90 degrees clockwise
              float tempX = uv1.x;
              uv1.x = uv1.y;
              uv1.y = uv2.x;
              uv2.x = uv2.y;
              uv2.y = tempX;
            } else if (rotation == 180) {
              // Rotate UV coordinates 180 degrees
              std::swap(uv1.x, uv2.x);
              std::swap(uv1.y, uv2.y);
            } else if (rotation == 270) {
              // Rotate UV coordinates 270 degrees clockwise
              float tempX = uv1.x;
              uv1.x = uv2.y;
              uv2.y = uv2.x;
              uv2.x = uv1.y;
              uv1.y = tempX;
            }
          }

          if (direction == "north") {
            // North face (-Z): Counter-clockwise from top-left
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.vertices.push_back(
                Vector3Transform({from.x, to.y, from.z}, transform));
            mesh.uvs.push_back({uv2.x, uv1.y});
            mesh.vertices.push_back(
                Vector3Transform({to.x, to.y, from.z}, transform));
            mesh.uvs.push_back({uv2.x, uv2.y});
            mesh.vertices.push_back(
                Vector3Transform({to.x, from.y, from.z}, transform));
            mesh.uvs.push_back({uv1.x, uv2.y});
            mesh.vertices.push_back(
                Vector3Transform({from.x, from.y, from.z}, transform));
          } else if (direction == "south") {
            // South face (+Z): Counter-clockwise from top-left
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.vertices.push_back(
                Vector3Transform({to.x, to.y, to.z}, transform));
            mesh.uvs.push_back({uv2.x, uv1.y});
            mesh.vertices.push_back(
                Vector3Transform({from.x, to.y, to.z}, transform));
            mesh.uvs.push_back({uv2.x, uv2.y});
            mesh.vertices.push_back(
                Vector3Transform({from.x, from.y, to.z}, transform));
            mesh.uvs.push_back({uv1.x, uv2.y});
            mesh.vertices.push_back(
                Vector3Transform({to.x, from.y, to.z}, transform));
          } else if (direction == "west") {
            // West face (-X): Counter-clockwise from top-left
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.vertices.push_back(
                Vector3Transform({from.x, to.y, to.z}, transform));
            mesh.uvs.push_back({uv2.x, uv1.y});
            mesh.vertices.push_back(
                Vector3Transform({from.x, to.y, from.z}, transform));
            mesh.uvs.push_back({uv2.x, uv2.y});
            mesh.vertices.push_back(
                Vector3Transform({from.x, from.y, from.z}, transform));
            mesh.uvs.push_back({uv1.x, uv2.y});
            mesh.vertices.push_back(
                Vector3Transform({from.x, from.y, to.z}, transform));
          } else if (direction == "east") {
            // East face (+X): Counter-clockwise from top-left
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.vertices.push_back(
                Vector3Transform({to.x, to.y, from.z}, transform));
            mesh.uvs.push_back({uv2.x, uv1.y});
            mesh.vertices.push_back(
                Vector3Transform({to.x, to.y, to.z}, transform));
            mesh.uvs.push_back({uv2.x, uv2.y});
            mesh.vertices.push_back(
                Vector3Transform({to.x, from.y, to.z}, transform));
            mesh.uvs.push_back({uv1.x, uv2.y});
            mesh.vertices.push_back(
                Vector3Transform({to.x, from.y, from.z}, transform));
          } else if (direction == "up") {
            // Up face (+Y): Counter-clockwise from top-left
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.vertices.push_back(
                Vector3Transform({from.x, to.y, from.z}, transform));
            mesh.uvs.push_back({uv1.x, uv2.y});
            mesh.vertices.push_back(
                Vector3Transform({from.x, to.y, to.z}, transform));
            mesh.uvs.push_back({uv2.x, uv2.y});
            mesh.vertices.push_back(
                Vector3Transform({to.x, to.y, to.z}, transform));
            mesh.uvs.push_back({uv2.x, uv1.y});
            mesh.vertices.push_back(
                Vector3Transform({to.x, to.y, from.z}, transform));
          } else if (direction == "down") {
            // Down face (-Y): Counter-clockwise from top-left
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.vertices.push_back(
                Vector3Transform({from.x, from.y, from.z}, transform));
            mesh.uvs.push_back({uv2.x, uv1.y});
            mesh.vertices.push_back(
                Vector3Transform({to.x, from.y, from.z}, transform));
            mesh.uvs.push_back({uv2.x, uv2.y});
            mesh.vertices.push_back(
                Vector3Transform({to.x, from.y, to.z}, transform));
            mesh.uvs.push_back({uv1.x, uv2.y});
            mesh.vertices.push_back(
                Vector3Transform({from.x, from.y, to.z}, transform));
          }
        }
      }
    }
  }
}

ResourceLocation Model::resolveTexture(const std::string &texture) {
  if (texture.find("#") != std::string::npos) {
    return resolveTexture(textures[texture.substr(1)]);
  }
  return ResourceLocation(texture);
}

void Model::draw(const Vector3 &position, const Vector3 &rotation,
                 const Vector3 &scale) {
  rlPushMatrix();
  rlTranslatef(position.x, position.y, position.z);
  rlRotatef(rotation.x, 1.0f, 0.0f, 0.0f);
  rlRotatef(rotation.y, 0.0f, 1.0f, 0.0f);
  rlRotatef(rotation.z, 0.0f, 0.0f, 1.0f);
  rlScalef(scale.x, scale.y, scale.z);

  // TODO: Apply rescale, somehow

  for (const auto &[tpath, mesh] : meshes) {
    const Texture2D &texture =
        TextureManager::getTexture(resolveTexture(tpath));
    rlSetTexture(texture.id);

    for (size_t i = 0; i < mesh.vertices.size(); i += 4) {
      // Draw first triangle (vertices 0, 1, 2)
      rlBegin(RL_TRIANGLES);
      for (size_t j = 0; j < 3; ++j) {
        rlTexCoord2f(mesh.uvs[i + j].x, mesh.uvs[i + j].y);
        rlVertex3f(mesh.vertices[i + j].x, mesh.vertices[i + j].y,
                   mesh.vertices[i + j].z);
      }
      rlEnd();
      
      // Draw second triangle (vertices 0, 2, 3)
      rlBegin(RL_TRIANGLES);
      // First vertex (0)
      rlTexCoord2f(mesh.uvs[i].x, mesh.uvs[i].y);
      rlVertex3f(mesh.vertices[i].x, mesh.vertices[i].y, mesh.vertices[i].z);
      
      // Third vertex (2)
      rlTexCoord2f(mesh.uvs[i + 2].x, mesh.uvs[i + 2].y);
      rlVertex3f(mesh.vertices[i + 2].x, mesh.vertices[i + 2].y, mesh.vertices[i + 2].z);
      
      // Fourth vertex (3)
      rlTexCoord2f(mesh.uvs[i + 3].x, mesh.uvs[i + 3].y);
      rlVertex3f(mesh.vertices[i + 3].x, mesh.vertices[i + 3].y, mesh.vertices[i + 3].z);
      rlEnd();
    }
    rlSetTexture(0);
  }

  rlPopMatrix();
}

} // namespace MCPSP
