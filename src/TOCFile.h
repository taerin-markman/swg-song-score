#pragma once
#include "stdafx.h"               /* Windows API             */
#include <stdio.h>
#include <vector>
#include "TREResource.h"

class TOCFile
{
protected:
  HANDLE fileHandle;
  DWORD fileSize;
  LPCSTR filePath;
  static void * mallocAndRead(HANDLE handle, SIZE_T bytesToRead);

public:
  unsigned int resourceCount;

  TOCFile(LPCSTR filePath);
  std::vector<TREResource *> * load(std::string filter);

};
