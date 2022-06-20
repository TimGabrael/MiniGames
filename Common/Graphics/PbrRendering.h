#pragma once

// returns Internal Object

void* CreateInternalPBRFromFile(const char* filename, float scale);
void CleanUpInternal(void* internalObj);

void InitializePbrPipeline();
void DrawPBRModel(void* internalObj);