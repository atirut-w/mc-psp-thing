#pragma once
#include "block.hpp"
#include "raylib.h"
#include "resource_location.hpp"
#include <array>
#include <string>
#include <unordered_map>

namespace MCPSP {

struct Mesh {
  std::vector<Vector3> vertices;
  std::vector<Vector2> uvs;
};

struct BlockState {
  ResourceLocation block = ResourceLocation("minecraft:air");
};

class Chunk {
  std::array<std::array<std::array<BlockState, 16>, 64>, 16> blocks;

  std::unordered_map<std::string, Mesh> meshes;
  bool dirty = true;

  void generateMesh();
  void generateBlockMesh(const BlockState &blockState, Vector3 position);

public:
  Chunk();

  void setBlock(int x, int y, int z, const ResourceLocation &block) {
    blocks[x][y][z].block = block;
    dirty = true;
  }

  void draw(const Vector3 &position);
};

} // namespace MCPSP
