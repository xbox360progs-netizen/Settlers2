#pragma once
#include <d3d9.h>

IDirect3DDevice9* GetGlobalDevice();
void SetGlobalDevice(IDirect3DDevice9* device);

void NaNCheckFloat(const char* name, float value);
void NaNCheckVector3(const char* name, const float* v);
void NaNCheckVector4(const char* name, const float* v);

bool IsNaN(float f);
bool IsINF(float f);
bool IsValid(float f);

void EnableNaNDetection(bool enable);
bool IsNaNDetectionEnabled();