#pragma once
#include "stdafx.h"               /* Windows API             */
#include <stdio.h>
#include <vector>
#include "TREResource.h"

class TREFile
{
protected:
  HANDLE fileHandle;
  DWORD fileSize;
  LPCSTR filePath;


public:
  unsigned int resourceCount;

  TREFile(LPCSTR filePath);
  std::vector<TREResource *> * load();
  static void * TREFile::mallocAndRead(HANDLE handle, SIZE_T bytesToRead, SIZE_T decompressedBytes);
  static void * TREFile::mallocAndRead(HANDLE handle, SIZE_T bytesToRead);

};