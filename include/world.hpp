#pragma once
#include "chunk.hpp"
#include <unordered_map>

namespace MCPSP {

struct ChunkPosition {
  int x, z;

  bool operator==(const ChunkPosition &other) const {
    return x == other.x && z == other.z;
  }
};

} // namespace MCPSP

namespace std {
template <> struct hash<MCPSP::ChunkPosition> {
  std::size_t operator()(const MCPSP::ChunkPosition &pos) const noexcept {
    return std::hash<int>()(pos.x) ^ (std::hash<int>()(pos.z) << 1);
  }
};
} // namespace std

namespace MCPSP {

class World {
private:
  std::unordered_map<ChunkPosition, Chunk> chunks;

public:
  World() = default;

  void generateChunk(int x, int z);

  void draw() {
    for (auto &[pos, chunk] : chunks) {
      Vector3 position = {static_cast<float>(pos.x * 16), 0.0f,
                          static_cast<float>(pos.z * 16)};
      chunk.draw(position);
    }
  }
};

} // namespace MCPSP
