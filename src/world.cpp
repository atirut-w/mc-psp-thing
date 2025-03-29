#include "world.hpp"
#include "chunk.hpp"

namespace MCPSP {

void World::generateChunk(int x, int z) {
  ChunkPosition pos{x, z};
  
  if (chunks.find(pos) == chunks.end()) {
    chunks[pos] = Chunk(this);
  }
  
  Chunk& chunk = chunks[pos];

  // TODO: More advanced terrain generation
  for (int i = 0; i < 16; ++i) {
    for (int j = 0; j < 64; ++j) {
      for (int k = 0; k < 16; ++k) {
        if (j == 0) {
          chunk.setBlock(i, j, k, ResourceLocation("minecraft:bedrock"));
        } else if (j < 10) {
          chunk.setBlock(i, j, k, ResourceLocation("minecraft:dirt"));
        } else if (j < 11) {
          chunk.setBlock(i, j, k, ResourceLocation("minecraft:grass_block"));
        } else {
          chunk.setBlock(i, j, k, ResourceLocation("minecraft:air"));
        }
      }
    }
  }
}

}
