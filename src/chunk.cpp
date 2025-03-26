#include "chunk.hpp"
#include "block_registry.hpp"
#include "resource_location.hpp"
#include "rlgl.h"
#include "texture_manager.hpp"
#include <GL/gl.h>
#include <raymath.h>

namespace MCPSP {

Chunk::Chunk() {}

void Chunk::generateMesh() {
  meshes.clear();

  for (int x = 0; x < 16; ++x) {
    for (int y = 0; y < 16; ++y) {
      for (int z = 0; z < 16; ++z) {
        BlockState &blockState = blocks[x][y][z];
        if (blockState.block != ResourceLocation("minecraft:air")) {
          Vector3 position = {static_cast<float>(x), static_cast<float>(y),
                              static_cast<float>(z)};
          generateBlockMesh(blockState, position);
        }
      }
    }
  }
}

void Chunk::generateBlockMesh(const BlockState &blockState, Vector3 position) {
  const Block &block = BlockRegistry::getBlock(blockState.block);
  const Model &model = block.model;

  for (const auto &element : model.getElements()) {
    Vector3 from = element.from;
    Vector3 to = element.to;

    Matrix transform = MatrixIdentity();
    
    // Apply element rotation if specified
    if (element.rotation.angle != 0.0f) {
      Vector3 origin = element.rotation.origin;
      
      // Apply rotation around the specified axis
      transform = MatrixTranslate(origin.x, origin.y, origin.z) * transform;
      
      if (element.rotation.axis == "x") {
        transform = MatrixRotateX(element.rotation.angle * DEG2RAD) * transform;
      } else if (element.rotation.axis == "y") {
        transform = MatrixRotateY(element.rotation.angle * DEG2RAD) * transform;
      } else if (element.rotation.axis == "z") {
        transform = MatrixRotateZ(element.rotation.angle * DEG2RAD) * transform;
      }
      
      transform = MatrixTranslate(-origin.x, -origin.y, -origin.z) * transform;
    }

    // Generate mesh for each face
    for (const auto &[direction, face] : element.faces) {
      // Skip face if it should be culled based on the cullface property
      if (!face.cullface.empty()) {
        bool shouldCull = false;
        
        // Determine neighboring block position based on cullface direction
        int nx = position.x;
        int ny = position.y;
        int nz = position.z;
        
        if (face.cullface == "north") nz -= 1;
        else if (face.cullface == "south") nz += 1;
        else if (face.cullface == "east") nx += 1;
        else if (face.cullface == "west") nx -= 1;
        else if (face.cullface == "up") ny += 1;
        else if (face.cullface == "down") ny -= 1;
        
        // Check if neighboring position is within chunk boundaries
        if (nx >= 0 && nx < 16 && ny >= 0 && ny < 16 && nz >= 0 && nz < 16) {
          // Check if neighboring block is solid (not air)
          if (blocks[nx][ny][nz].block != ResourceLocation("minecraft:air")) {
            shouldCull = true;
          }
        }
        
        // Skip this face if it's culled
        if (shouldCull) {
          continue;
        }
      }
      
      ResourceLocation texture = model.resolveTexture(face.texture);
      if (meshes.find(texture) == meshes.end()) {
        meshes[texture] = Mesh();
      }
      Mesh &mesh = meshes[texture];

      Vector2 uv1 = face.uv1;
      Vector2 uv2 = face.uv2;

      // Apply face rotation if specified
      if (face.rotation != 0) {
        if (face.rotation == 90) {
          // Rotate UV coordinates 90 degrees clockwise
          float tempX = uv1.x;
          uv1.x = uv1.y;
          uv1.y = uv2.x;
          uv2.x = uv2.y;
          uv2.y = tempX;
        } else if (face.rotation == 180) {
          // Rotate UV coordinates 180 degrees
          std::swap(uv1.x, uv2.x);
          std::swap(uv1.y, uv2.y);
        } else if (face.rotation == 270) {
          // Rotate UV coordinates 270 degrees clockwise
          float tempX = uv1.x;
          uv1.x = uv2.y;
          uv2.y = uv2.x;
          uv2.x = uv1.y;
          uv1.y = tempX;
        }
      }

      // Create vertices based on direction
      if (direction == "north") {
        // North face (-Z): Counter-clockwise from top-left
        Vector3 v1 = {from.x, to.y, from.z};   // Top-left
        Vector3 v2 = {to.x, to.y, from.z};     // Top-right
        Vector3 v3 = {to.x, from.y, from.z};   // Bottom-right
        Vector3 v4 = {from.x, from.y, from.z}; // Bottom-left

        // Apply transformation
        v1 = Vector3Transform(v1, transform);
        v2 = Vector3Transform(v2, transform);
        v3 = Vector3Transform(v3, transform);
        v4 = Vector3Transform(v4, transform);

        // Translate to block position
        v1 = {v1.x + position.x, v1.y + position.y, v1.z + position.z};
        v2 = {v2.x + position.x, v2.y + position.y, v2.z + position.z};
        v3 = {v3.x + position.x, v3.y + position.y, v3.z + position.z};
        v4 = {v4.x + position.x, v4.y + position.y, v4.z + position.z};

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
        Vector3 v1 = {to.x, to.y, to.z};     // Top-left
        Vector3 v2 = {from.x, to.y, to.z};   // Top-right
        Vector3 v3 = {from.x, from.y, to.z}; // Bottom-right
        Vector3 v4 = {to.x, from.y, to.z};   // Bottom-left

        // Apply transformation
        v1 = Vector3Transform(v1, transform);
        v2 = Vector3Transform(v2, transform);
        v3 = Vector3Transform(v3, transform);
        v4 = Vector3Transform(v4, transform);

        // Translate to block position
        v1 = {v1.x + position.x, v1.y + position.y, v1.z + position.z};
        v2 = {v2.x + position.x, v2.y + position.y, v2.z + position.z};
        v3 = {v3.x + position.x, v3.y + position.y, v3.z + position.z};
        v4 = {v4.x + position.x, v4.y + position.y, v4.z + position.z};

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
        Vector3 v1 = {to.x, to.y, from.z};   // Top-left
        Vector3 v2 = {to.x, to.y, to.z};     // Top-right
        Vector3 v3 = {to.x, from.y, to.z};   // Bottom-right
        Vector3 v4 = {to.x, from.y, from.z}; // Bottom-left

        // Apply transformation
        v1 = Vector3Transform(v1, transform);
        v2 = Vector3Transform(v2, transform);
        v3 = Vector3Transform(v3, transform);
        v4 = Vector3Transform(v4, transform);

        // Translate to block position
        v1 = {v1.x + position.x, v1.y + position.y, v1.z + position.z};
        v2 = {v2.x + position.x, v2.y + position.y, v2.z + position.z};
        v3 = {v3.x + position.x, v3.y + position.y, v3.z + position.z};
        v4 = {v4.x + position.x, v4.y + position.y, v4.z + position.z};

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
        Vector3 v1 = {from.x, to.y, to.z};     // Top-left
        Vector3 v2 = {from.x, to.y, from.z};   // Top-right
        Vector3 v3 = {from.x, from.y, from.z}; // Bottom-right
        Vector3 v4 = {from.x, from.y, to.z};   // Bottom-left

        // Apply transformation
        v1 = Vector3Transform(v1, transform);
        v2 = Vector3Transform(v2, transform);
        v3 = Vector3Transform(v3, transform);
        v4 = Vector3Transform(v4, transform);

        // Translate to block position
        v1 = {v1.x + position.x, v1.y + position.y, v1.z + position.z};
        v2 = {v2.x + position.x, v2.y + position.y, v2.z + position.z};
        v3 = {v3.x + position.x, v3.y + position.y, v3.z + position.z};
        v4 = {v4.x + position.x, v4.y + position.y, v4.z + position.z};

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

        // Translate to block position
        v1 = {v1.x + position.x, v1.y + position.y, v1.z + position.z};
        v2 = {v2.x + position.x, v2.y + position.y, v2.z + position.z};
        v3 = {v3.x + position.x, v3.y + position.y, v3.z + position.z};
        v4 = {v4.x + position.x, v4.y + position.y, v4.z + position.z};

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

        // Translate to block position
        v1 = {v1.x + position.x, v1.y + position.y, v1.z + position.z};
        v2 = {v2.x + position.x, v2.y + position.y, v2.z + position.z};
        v3 = {v3.x + position.x, v3.y + position.y, v3.z + position.z};
        v4 = {v4.x + position.x, v4.y + position.y, v4.z + position.z};

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

void Chunk::draw(const Vector3 &position) {
  if (dirty) {
    generateMesh();
    dirty = false;
  }

  rlPushMatrix();
  rlTranslatef(position.x, position.y, position.z);

  for (const auto &[texture, mesh] : meshes) {
    const Texture2D &tex = TextureManager::getTexture(texture);
    rlSetTexture(tex.id);

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
