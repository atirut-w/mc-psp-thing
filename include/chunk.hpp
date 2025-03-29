#pragma once
#include "block.hpp"
#include "raylib.h"
#include "resource_location.hpp"
#include <array>
#include <string>
#include <unordered_map>

namespace MCPSP {

class World;

struct Mesh {
  std::vector<Vector3> vertices;
  std::vector<Vector2> uvs;
};

struct BlockState {
  ResourceLocation block = ResourceLocation("minecraft:air");
};

class Chunk {
  World *world;
  std::array<std::array<std::array<BlockState, 16>, 64>, 16> blocks;

  // Position of this chunk in the world grid
  int chunkX;
  int chunkZ;

  std::unordered_map<std::string, Mesh> meshes;
  bool dirty = true;

  void generateMesh();
  void generateBlockMesh(const BlockState &blockState, Vector3 position);

public:
  Chunk() = default;
  Chunk(World *world, int chunkX, int chunkZ)
      : world(world), chunkX(chunkX), chunkZ(chunkZ) {};

  void setBlock(int x, int y, int z, const ResourceLocation &block) {
    blocks[x][y][z].block = block;
    dirty = true;
  }

  BlockState getBlock(int x, int y, int z) const {
    if (x >= 0 && x < 16 && y >= 0 && y < 64 && z >= 0 && z < 16) {
      return blocks[x][y][z];
    }
    return BlockState(); // Returns air by default
  }

  // Get chunk position
  int getChunkX() const { return chunkX; }
  int getChunkZ() const { return chunkZ; }

  void draw(const Vector3 &position);
};

} // namespace MCPSP
