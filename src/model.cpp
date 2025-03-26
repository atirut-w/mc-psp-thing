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
        transform =
            MatrixTranslate(-origin.x, -origin.y, -origin.z) * transform;
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
            // Generate UVs based on model coordinates
            // Note: from/to are already normalized to 0-1 range (divided by 16)
            if (direction == "north") {
              uv1 = {to.x, 1.0f - to.y};     // Top-right
              uv2 = {from.x, 1.0f - from.y}; // Bottom-left
            } else if (direction == "south") {
              uv1 = {from.x, 1.0f - to.y}; // Top-left
              uv2 = {to.x, 1.0f - from.y}; // Bottom-right
            } else if (direction == "west") {
              uv1 = {to.z, 1.0f - to.y};     // Top-right
              uv2 = {from.z, 1.0f - from.y}; // Bottom-left
            } else if (direction == "east") {
              uv1 = {from.z, 1.0f - to.y}; // Top-left
              uv2 = {to.z, 1.0f - from.y}; // Bottom-right
            } else if (direction == "up") {
              uv1 = {from.x, from.z}; // Top-left
              uv2 = {to.x, to.z};     // Bottom-right
            } else if (direction == "down") {
              uv1 = {from.x, to.z}; // Top-left
              uv2 = {to.x, from.z}; // Bottom-right
            }
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
            Vector3 v1 = {from.x, to.y, from.z};  // Top-left
            Vector3 v2 = {to.x, to.y, from.z};    // Top-right
            Vector3 v3 = {to.x, from.y, from.z};  // Bottom-right
            Vector3 v4 = {from.x, from.y, from.z}; // Bottom-left

            // Apply transformation
            v1 = Vector3Transform(v1, transform);
            v2 = Vector3Transform(v2, transform);
            v3 = Vector3Transform(v3, transform);
            v4 = Vector3Transform(v4, transform);

            // Add first triangle (v1, v2, v3)
            mesh.vertices.push_back(v1);
            mesh.vertices.push_back(v2);
            mesh.vertices.push_back(v3);
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.uvs.push_back({uv2.x, uv1.y});
            mesh.uvs.push_back({uv2.x, uv2.y});

            // Add second triangle (v1, v3, v4)
            mesh.vertices.push_back(v1);
            mesh.vertices.push_back(v3);
            mesh.vertices.push_back(v4);
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.uvs.push_back({uv2.x, uv2.y});
            mesh.uvs.push_back({uv1.x, uv2.y});
          } else if (direction == "south") {
            // South face (+Z): Counter-clockwise from top-left
            Vector3 v1 = {to.x, to.y, to.z};    // Top-left
            Vector3 v2 = {from.x, to.y, to.z};  // Top-right
            Vector3 v3 = {from.x, from.y, to.z}; // Bottom-right
            Vector3 v4 = {to.x, from.y, to.z};  // Bottom-left

            // Apply transformation
            v1 = Vector3Transform(v1, transform);
            v2 = Vector3Transform(v2, transform);
            v3 = Vector3Transform(v3, transform);
            v4 = Vector3Transform(v4, transform);

            // Add triangles
            mesh.vertices.push_back(v1);
            mesh.vertices.push_back(v2);
            mesh.vertices.push_back(v3);
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.uvs.push_back({uv2.x, uv1.y});
            mesh.uvs.push_back({uv2.x, uv2.y});

            mesh.vertices.push_back(v1);
            mesh.vertices.push_back(v3);
            mesh.vertices.push_back(v4);
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.uvs.push_back({uv2.x, uv2.y});
            mesh.uvs.push_back({uv1.x, uv2.y});
          } else if (direction == "east") {
            // East face (+X): Counter-clockwise from top-left
            Vector3 v1 = {to.x, to.y, from.z};  // Top-left
            Vector3 v2 = {to.x, to.y, to.z};    // Top-right
            Vector3 v3 = {to.x, from.y, to.z};  // Bottom-right
            Vector3 v4 = {to.x, from.y, from.z}; // Bottom-left

            // Apply transformation
            v1 = Vector3Transform(v1, transform);
            v2 = Vector3Transform(v2, transform);
            v3 = Vector3Transform(v3, transform);
            v4 = Vector3Transform(v4, transform);

            // Add triangles
            mesh.vertices.push_back(v1);
            mesh.vertices.push_back(v2);
            mesh.vertices.push_back(v3);
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.uvs.push_back({uv2.x, uv1.y});
            mesh.uvs.push_back({uv2.x, uv2.y});

            mesh.vertices.push_back(v1);
            mesh.vertices.push_back(v3);
            mesh.vertices.push_back(v4);
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.uvs.push_back({uv2.x, uv2.y});
            mesh.uvs.push_back({uv1.x, uv2.y});
          } else if (direction == "west") {
            // West face (-X): Counter-clockwise from top-left
            Vector3 v1 = {from.x, to.y, to.z};    // Top-left
            Vector3 v2 = {from.x, to.y, from.z};  // Top-right
            Vector3 v3 = {from.x, from.y, from.z}; // Bottom-right
            Vector3 v4 = {from.x, from.y, to.z};  // Bottom-left

            // Apply transformation
            v1 = Vector3Transform(v1, transform);
            v2 = Vector3Transform(v2, transform);
            v3 = Vector3Transform(v3, transform);
            v4 = Vector3Transform(v4, transform);

            // Add triangles
            mesh.vertices.push_back(v1);
            mesh.vertices.push_back(v2);
            mesh.vertices.push_back(v3);
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.uvs.push_back({uv2.x, uv1.y});
            mesh.uvs.push_back({uv2.x, uv2.y});

            mesh.vertices.push_back(v1);
            mesh.vertices.push_back(v3);
            mesh.vertices.push_back(v4);
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.uvs.push_back({uv2.x, uv2.y});
            mesh.uvs.push_back({uv1.x, uv2.y});
          } else if (direction == "up") {
            // Up face (+Y): Counter-clockwise from top-left
            Vector3 v1 = {from.x, to.y, from.z}; // Top-left
            Vector3 v2 = {from.x, to.y, to.z};   // Top-right
            Vector3 v3 = {to.x, to.y, to.z};     // Bottom-right
            Vector3 v4 = {to.x, to.y, from.z};   // Bottom-left

            // Apply transformation
            v1 = Vector3Transform(v1, transform);
            v2 = Vector3Transform(v2, transform);
            v3 = Vector3Transform(v3, transform);
            v4 = Vector3Transform(v4, transform);

            // Add triangles
            mesh.vertices.push_back(v1);
            mesh.vertices.push_back(v2);
            mesh.vertices.push_back(v3);
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.uvs.push_back({uv1.x, uv2.y});
            mesh.uvs.push_back({uv2.x, uv2.y});

            mesh.vertices.push_back(v1);
            mesh.vertices.push_back(v3);
            mesh.vertices.push_back(v4);
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.uvs.push_back({uv2.x, uv2.y});
            mesh.uvs.push_back({uv2.x, uv1.y});
          } else if (direction == "down") {
            // Down face (-Y): Counter-clockwise from top-left
            Vector3 v1 = {from.x, from.y, to.z};   // Top-left
            Vector3 v2 = {from.x, from.y, from.z}; // Top-right
            Vector3 v3 = {to.x, from.y, from.z};   // Bottom-right
            Vector3 v4 = {to.x, from.y, to.z};     // Bottom-left

            // Apply transformation
            v1 = Vector3Transform(v1, transform);
            v2 = Vector3Transform(v2, transform);
            v3 = Vector3Transform(v3, transform);
            v4 = Vector3Transform(v4, transform);

            // Add triangles
            mesh.vertices.push_back(v1);
            mesh.vertices.push_back(v2);
            mesh.vertices.push_back(v3);
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.uvs.push_back({uv1.x, uv2.y});
            mesh.uvs.push_back({uv2.x, uv2.y});

            mesh.vertices.push_back(v1);
            mesh.vertices.push_back(v3);
            mesh.vertices.push_back(v4);
            mesh.uvs.push_back({uv1.x, uv1.y});
            mesh.uvs.push_back({uv2.x, uv2.y});
            mesh.uvs.push_back({uv2.x, uv1.y});
          }
        }
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

void Model::draw(const Vector3 &position, const Vector3 &rotation,
                 const Vector3 &scale) const {
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

    // Enable alpha testing for transparency
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f); // Discard pixels with alpha < 0.1

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, mesh.vertices.data());
    glTexCoordPointer(2, GL_FLOAT, 0, mesh.uvs.data());
    glDrawArrays(GL_TRIANGLES, 0, mesh.vertices.size());

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    // Disable alpha testing when done
    glDisable(GL_ALPHA_TEST);
    rlSetTexture(0);
  }

  rlPopMatrix();
}

} // namespace MCPSP
