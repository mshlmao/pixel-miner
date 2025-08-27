#pragma once
#include <unordered_map>

#include "raylib.h"

const unsigned char CHUNK_SIZE = 64;

struct ChunkId {
  int x;
  int y;

  bool operator==(const ChunkId &other) const {
    return x == other.x && y == other.y;
  }
};

template <>
struct std::hash<ChunkId> {
  size_t operator()(const ChunkId &id) const {
    size_t hashX = std::hash<int>{}(id.x);
    size_t hashY = std::hash<int>{}(id.y);
    return hashX ^ (hashY << 1);
  }
};

struct Chunk {
  ChunkId id;
  bool isReady;
  bool needsUpdate;  // When minedPixels change image needsan update
  bool minedPixels[CHUNK_SIZE][CHUNK_SIZE] = {false};
  Image chunkImage;
  Texture chunkTexture;
  // 0 = not mined, 1 = mined
};