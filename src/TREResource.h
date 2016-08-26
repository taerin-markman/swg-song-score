#pragma once
#include "stdafx.h"

class TREResource
{
protected:
  DWORD32 dataSize;
  DWORD32 dataOffset;
  DWORD32 dataCompressed;
  DWORD32 dataCompressedSize;
  LPSTR filePath;

public:
  LPSTR name;

  TREResource(DWORD32 dataSize,
    DWORD32 dataOffset,
    DWORD32 dataCompressed,
    DWORD32 dataCompressedSize,
    LPCSTR filePath,
    LPCSTR name
  );

  DWORD load(void **);
};
