#include "chunkManager.hpp"

#include <cmath>
#include <unordered_map>
#include <vector>

#include "chunk.hpp"
#include "raylib.h"
#include "raymath.h"

const unsigned char CHUNK_RENDER_DIISTANCE =
    9;  // Greater than 0, ODD (otherwise people WILL DIE)
const unsigned char CHUNK_RANGE =
    (CHUNK_RENDER_DIISTANCE - (CHUNK_RENDER_DIISTANCE % 2)) / 2;

std::unordered_map<ChunkId, Chunk> loadedChunks;
std::vector<ChunkId> chunksToUnload;

Shader chunkShader;

ChunkId WorldCoordToChunkId(Vector2 worldCoord) {
  ChunkId resultChunkId;
  resultChunkId.x = (int)std::floorf(worldCoord.x / (float)CHUNK_SIZE);
  resultChunkId.y = (int)std::floorf(worldCoord.y / (float)CHUNK_SIZE);
  return resultChunkId;
}

bool IsChunkActive(ChunkId chunkId) { return loadedChunks.contains(chunkId); }

Chunk &GetActiveChunk(ChunkId chunkId) { return loadedChunks.at(chunkId); }

Image GenerateChunkImage(ChunkId chunkId) {  // *"World generation"
  //! not using for now, this code may be moved to gpu shader for performance
  Image noiseImage1 =
      GenImagePerlinNoise(CHUNK_SIZE, CHUNK_SIZE, chunkId.x * CHUNK_SIZE,
                          chunkId.y * CHUNK_SIZE, 8);
  Image noiseImage2 =
      GenImagePerlinNoise(CHUNK_SIZE, CHUNK_SIZE, chunkId.x * CHUNK_SIZE,
                          chunkId.y * CHUNK_SIZE, 48);
  Image chunkImage = GenImageColor(CHUNK_SIZE, CHUNK_SIZE, (Color){0, 0, 0, 0});
  unsigned char pixelColor = 0;
  for (int j = 0; j < CHUNK_SIZE; j++) {
    for (int i = 0; i < CHUNK_SIZE; i++) {
      pixelColor = (unsigned char)(GetImageColor(noiseImage1, i, j).r * 0.8f +
                                   GetImageColor(noiseImage2, i, j).r * 0.2f);
                                   
      if (pixelColor > 128) {
        ImageDrawPixel(&chunkImage, i, j, (Color){74, 51, 45, 255});
      } else if (pixelColor > 48) {
        ImageDrawPixel(&chunkImage, i, j, (Color){108, 74, 63, 255});
      } else if (pixelColor > 32) {
        ImageDrawPixel(&chunkImage, i, j, (Color){208, 199, 194, 255});
      } else {
        ImageDrawPixel(&chunkImage, i, j, (Color){245, 242, 234, 255});
      }
    }
  }
  UnloadImage(noiseImage1);
  UnloadImage(noiseImage2);
  return chunkImage;
}

void LoadChunkImage(ChunkId chunkId) {
  // * Do not use unless initializing a chunk (generates image from scratch)
  Chunk &chunkToLoadImage = GetActiveChunk(chunkId);

  if (IsImageValid(chunkToLoadImage.chunkImage)) {
    UnloadImage(chunkToLoadImage.chunkImage);
  }
  chunkToLoadImage.chunkImage = GenerateChunkImage(chunkId);
}

void LoadChunkTexture(ChunkId chunkId) {
  Chunk &chunkToLoadTexture = GetActiveChunk(chunkId);

  if (IsTextureValid(chunkToLoadTexture.chunkTexture)) {
    UpdateTexture(chunkToLoadTexture.chunkTexture,
                  chunkToLoadTexture.chunkImage.data);
  } else {
    chunkToLoadTexture.chunkTexture =
        LoadTextureFromImage(chunkToLoadTexture.chunkImage);
    SetTextureWrap(chunkToLoadTexture.chunkTexture, TEXTURE_WRAP_CLAMP);
  }
}

void LoadChunk(ChunkId chunkId) {
  if (IsChunkActive(chunkId)) {
    TraceLog(LOG_ERROR,
             "Chunk: %d, %d is already loaded and will not be loaded again.",
             chunkId.x, chunkId.y);
    return;
  }

  loadedChunks[chunkId];
  loadedChunks[chunkId].id = chunkId;

  // TODO: retrieve mined pixels from a save file i guess
  LoadChunkImage(chunkId);
  loadedChunks[chunkId].needsUpdate = true;
  return;
}

void UnloadChunk(ChunkId chunkId) {
  Chunk &chunkToUnload = GetActiveChunk(chunkId);
  if (IsImageValid(chunkToUnload.chunkImage)) {
    UnloadImage(chunkToUnload.chunkImage);
  }
  if (IsTextureValid(chunkToUnload.chunkTexture)) {
    UnloadTexture(chunkToUnload.chunkTexture);
  }
  loadedChunks.erase(chunkId);
}

void InitializeChunks() {
  chunkShader = LoadShader(0, "res\\shaders\\chunkShader.fs");
}

void UpdateChunkImage(ChunkId chunkId) {  // Doesnt generate an image
  Chunk &chunkToUpdateImage = GetActiveChunk(chunkId);

  for (int j = 0; j < CHUNK_SIZE; j++) {
    for (int i = 0; i < CHUNK_SIZE; i++) {
      Color pixelColor = GetImageColor(chunkToUpdateImage.chunkImage, i, j);
      if (chunkToUpdateImage.minedPixels[i][j] == true) {
        pixelColor.a = 0;
        ImageDrawPixel(&chunkToUpdateImage.chunkImage, i, j, pixelColor);
      } else {
        pixelColor.a = 255;
        ImageDrawPixel(&chunkToUpdateImage.chunkImage, i, j, pixelColor);
      }
    }
  }

  LoadChunkTexture(chunkId);
}

void UpdateDynamicChunkLoading(Vector2 cameraPositionWorld) {
  ChunkId cameraChunkId = WorldCoordToChunkId(cameraPositionWorld);
  // Optionally check if caera chunk changed and if not break

  for (auto &[chunkId, chunk] : loadedChunks) {
    if (chunk.id.x < cameraChunkId.x - CHUNK_RANGE ||
        chunk.id.x > cameraChunkId.x + CHUNK_RANGE ||
        chunk.id.y < cameraChunkId.y - CHUNK_RANGE ||
        chunk.id.y > cameraChunkId.y + CHUNK_RANGE) {
      chunksToUnload.emplace_back(chunkId);
    }
  }

  for (auto &chunkId : chunksToUnload) {
    UnloadChunk(chunkId);
  }
  chunksToUnload.clear();

  for (int dx = -CHUNK_RANGE; dx <= CHUNK_RANGE; dx++) {
    for (int dy = -CHUNK_RANGE; dy <= CHUNK_RANGE; dy++) {
      if (!IsChunkActive(ChunkId{cameraChunkId.x - dx, cameraChunkId.y - dy})) {
        LoadChunk(ChunkId{cameraChunkId.x - dx, cameraChunkId.y - dy});
      }
    }
  }
}

void UpdateChunks(Vector2 cameraPositionWorld) {
  UpdateDynamicChunkLoading(cameraPositionWorld);

  for (auto &[chunkId, chunk] : loadedChunks) {
    if (chunk.needsUpdate) {
      UpdateChunkImage(chunk.id);
      chunk.needsUpdate = false;
    }
  }
}

void DrawChunks() {
  BeginShaderMode(chunkShader);
  for (auto &[chunkId, chunk] : loadedChunks) {
    if (!IsChunkActive(chunkId)) {
      continue;
    }

    DrawTexturePro(
        chunk.chunkTexture,
        (Rectangle){0.0f, 0.0f, (float)CHUNK_SIZE, (float)CHUNK_SIZE},
        (Rectangle){(float)(chunk.id.x * CHUNK_SIZE),
                    (float)(chunk.id.y * CHUNK_SIZE), (float)(CHUNK_SIZE),
                    (float)(CHUNK_SIZE)},
        (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
  }
  EndShaderMode();
}

void DrawChunkBorders(Vector2 camPos) {
  for (auto &[chunkId, chunk] : loadedChunks) {
    if (!IsChunkActive(chunkId)) {
      continue;
    }

    Chunk chunkToDraw = chunk;

    DrawLine(chunkToDraw.id.x * CHUNK_SIZE, chunkToDraw.id.y * CHUNK_SIZE,
             chunkToDraw.id.x * CHUNK_SIZE, (chunkToDraw.id.y + 1) * CHUNK_SIZE,
             MAGENTA);
    DrawLine(chunkToDraw.id.x * CHUNK_SIZE, chunkToDraw.id.y * CHUNK_SIZE,
             (chunkToDraw.id.x + 1) * CHUNK_SIZE, chunkToDraw.id.y * CHUNK_SIZE,
             MAGENTA);
    DrawLine(chunkToDraw.id.x * CHUNK_SIZE, (chunkToDraw.id.y + 1) * CHUNK_SIZE,
             (chunkToDraw.id.x + 1) * CHUNK_SIZE,
             (chunkToDraw.id.y + 1) * CHUNK_SIZE, MAGENTA);
    DrawLine((chunkToDraw.id.x + 1) * CHUNK_SIZE, chunkToDraw.id.y * CHUNK_SIZE,
             (chunkToDraw.id.x + 1) * CHUNK_SIZE,
             (chunkToDraw.id.y + 1) * CHUNK_SIZE, MAGENTA);
    DrawLine(chunkToDraw.id.x * CHUNK_SIZE, chunkToDraw.id.y * CHUNK_SIZE,
             (chunkToDraw.id.x + 1) * CHUNK_SIZE,
             (chunkToDraw.id.y + 1) * CHUNK_SIZE, MAGENTA);
  }
  Chunk chunkToDraw = GetActiveChunk(WorldCoordToChunkId(camPos));
  DrawLine(chunkToDraw.id.x * CHUNK_SIZE, chunkToDraw.id.y * CHUNK_SIZE,
           chunkToDraw.id.x * CHUNK_SIZE, (chunkToDraw.id.y + 1) * CHUNK_SIZE,
           GREEN);
  DrawLine(chunkToDraw.id.x * CHUNK_SIZE, chunkToDraw.id.y * CHUNK_SIZE,
           (chunkToDraw.id.x + 1) * CHUNK_SIZE, chunkToDraw.id.y * CHUNK_SIZE,
           GREEN);
  DrawLine(chunkToDraw.id.x * CHUNK_SIZE, (chunkToDraw.id.y + 1) * CHUNK_SIZE,
           (chunkToDraw.id.x + 1) * CHUNK_SIZE,
           (chunkToDraw.id.y + 1) * CHUNK_SIZE, GREEN);
  DrawLine((chunkToDraw.id.x + 1) * CHUNK_SIZE, chunkToDraw.id.y * CHUNK_SIZE,
           (chunkToDraw.id.x + 1) * CHUNK_SIZE,
           (chunkToDraw.id.y + 1) * CHUNK_SIZE, GREEN);
  DrawLine(chunkToDraw.id.x * CHUNK_SIZE, chunkToDraw.id.y * CHUNK_SIZE,
           (chunkToDraw.id.x + 1) * CHUNK_SIZE,
           (chunkToDraw.id.y + 1) * CHUNK_SIZE, GREEN);
}

void DestroyPixel(ChunkId chunkId, unsigned char pixelCoordX,
                  unsigned char pixelCoordY) {
  // TODO: make it not crash when trying to acces a chunk/pixel that doesnt
  // TODO: exist
  if (!IsChunkActive(chunkId)) {
    return;
  }

  GetActiveChunk(chunkId).minedPixels[pixelCoordX][pixelCoordY] = true;
  GetActiveChunk(chunkId).needsUpdate = true;
}

void WorldDestroyPixelAt(Vector2 worldCoord) {
  ChunkId targetChunkId = WorldCoordToChunkId(worldCoord);
  int pixelCoordX = ((int)std::floorf(worldCoord.x) % CHUNK_SIZE);
  int pixelCoordY = ((int)std::floorf(worldCoord.y) % CHUNK_SIZE);
  if (pixelCoordX < 0) {
    pixelCoordX += CHUNK_SIZE;
  }
  if (pixelCoordY < 0) {
    pixelCoordY += CHUNK_SIZE;
  }

  DestroyPixel(targetChunkId, pixelCoordX, pixelCoordY);
}

void ResetAllChunks() {
  for (auto &[chunkId, chunk] : loadedChunks)
    for (int x = 0; x < CHUNK_SIZE; x++) {
      for (int y = 0; y < CHUNK_SIZE; y++) {
        chunk.minedPixels[x][y] = false;
      }
      chunk.needsUpdate = true;
    }
}

bool CheckCollisionRectChunk(Rectangle collider, ChunkId chunkId) {
  Vector2 startPixelCoord = Vector2{collider.x - chunkId.x * CHUNK_SIZE,
                                    collider.y - chunkId.y * CHUNK_SIZE};
  Vector2 endPixelCoord =
      Vector2{collider.x + collider.width - chunkId.x * CHUNK_SIZE,
              collider.y + collider.height - chunkId.y * CHUNK_SIZE};
  if (startPixelCoord.x < 0) startPixelCoord.x = 0;
  if (startPixelCoord.y < 0) startPixelCoord.y = 0;
  if (endPixelCoord.x > CHUNK_SIZE) endPixelCoord.x = CHUNK_SIZE;
  if (endPixelCoord.y > CHUNK_SIZE) endPixelCoord.y = CHUNK_SIZE;

  int startX = (int)std::floorf(startPixelCoord.x);
  int endX = (int)std::ceilf(endPixelCoord.x);
  int startY = (int)std::floorf(startPixelCoord.y);
  int endY = (int)std::ceilf(endPixelCoord.y);

  // Clamp integers for safety (in case of floating-point precision issues)
  if (startX < 0) startX = 0;
  if (startY < 0) startY = 0;
  if (endX > CHUNK_SIZE) endX = CHUNK_SIZE;
  if (endY > CHUNK_SIZE) endY = CHUNK_SIZE;

  for (int y = startY; y < endY; y++) {
    for (int x = startX; x < endX; x++) {
      if (GetActiveChunk(chunkId).minedPixels[x][y] == false) {
        return true;
      }
    }
  }
  return false;
}

bool CheckCollisionRect(Rectangle collider) {
  ChunkId startChunkId = WorldCoordToChunkId(Vector2{collider.x, collider.y});
  ChunkId endChunkId = WorldCoordToChunkId(
      Vector2{collider.x + collider.width, collider.y + collider.height});
  for (int y = startChunkId.y; y <= endChunkId.y; y++) {
    for (int x = startChunkId.x; x <= endChunkId.x; x++) {
      if (CheckCollisionRectChunk(collider, ChunkId{x, y})) {
        return true;
      }
    }
  }
  return false;
}