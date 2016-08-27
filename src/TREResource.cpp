#include "stdafx.h"
#include <stdio.h>
#include "TREResource.h"
#include "TREFile.h"
#include "Debug.h"

TREResource::TREResource(
  DWORD32 dataSize,
      DWORD32 dataOffset,
      DWORD32 dataCompressed,
      DWORD32 dataCompressedSize,
      LPCSTR filePath,
      LPCSTR name
    )
{
  this->dataSize = dataSize;
  this->dataOffset = dataOffset;
  this->dataCompressed = dataCompressed;
  this->dataCompressedSize = dataCompressedSize;

  SIZE_T filePathSize = strlen(filePath) + 1;
  this->filePath = new CHAR[filePathSize];
  strncpy_s(this->filePath, filePathSize, filePath, filePathSize);

  SIZE_T nameSize = strlen(name) + 1;
  this->name = new CHAR[nameSize];
  strncpy_s(this->name, nameSize, name, nameSize);
}

DWORD TREResource::load(void **out)
{
  *out = NULL;
  bool success = true;

  HANDLE fileHandle = CreateFile(
    this->filePath,
    GENERIC_READ,
    FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    0,
    NULL
  );

  if (fileHandle == INVALID_HANDLE_VALUE)
  {
    success = false;
    TRACE("Could not open file [%s]", this->filePath);
  }

  if (success)
  {
    SetFilePointer(fileHandle, this->dataOffset, 0, FILE_BEGIN);

    DWORD bytesToRead = this->dataSize;
    if (this->dataCompressed != 0)
    {
      bytesToRead = this->dataCompressedSize;
    }

    TRACE("Reading contents of [%s], compressed=%d, uncompressed=%d", this->filePath, bytesToRead, this->dataSize);
    *out = TREFile::mallocAndRead(fileHandle, bytesToRead, this->dataSize);
  }

  return this->dataSize;
}
