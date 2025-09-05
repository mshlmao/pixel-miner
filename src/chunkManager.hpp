#pragma once

#include "chunk.hpp"
#include "raylib.h"

ChunkId WorldCoordToChunkId(Vector2 worldCoord);
void InitializeChunks();
void UpdateChunks(Vector2 cameraWorldPosition);
void DrawChunks();
void DrawChunkBorders(Vector2 cameraWorldPosition);  // *DEBUG
void WorldDestroyPixelAt(Vector2 worldCoord);
void ResetAllChunks();  // *DEBUG


bool CheckCollisionRect(Rectangle colliderRect);
