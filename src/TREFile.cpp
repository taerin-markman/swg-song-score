#include "stdafx.h"               /* Windows API             */
#include "..\zlib\include\zlib.h" /* Zlib Inflation          */
#include <stdio.h>

#include "Debug.h"
#include "TREFile.h"

//#pragma comment(linker,"zdll.lib")

#pragma pack(push,1)
typedef struct
{
  char fourcc[4];
  char version[4];
  DWORD32 count;
  DWORD32 infoOffset;
  DWORD32 infoCompressed;
  DWORD32 infoCompressedSize;
  DWORD32 namesCompressed;
  DWORD32 namesCompressedSize;
  DWORD32 namesSize;
} TREHeaderType;

typedef struct
{
  DWORD32 checksum;
  DWORD32 dataSize;
  DWORD32 dataOffset;
  DWORD32 dataCompressed;
  DWORD32 dataCompressedSize;
  DWORD32 nameOffset;
} TREInfoHeaderType;
#pragma pack(pop)

TREFile::TREFile(LPCSTR filePath)
{
  this->resourceCount = 0;
  this->filePath = filePath;
  this->fileHandle = INVALID_HANDLE_VALUE;
}

void * TREFile::mallocAndRead(HANDLE handle, SIZE_T bytesToRead)
{
  BOOL readSuccess = FALSE;
  DWORD bytesRead = 0;
  void *data = HeapAlloc(GetProcessHeap(), 0, bytesToRead);

  readSuccess = ReadFile(
    handle,
    data,
    bytesToRead,
    &bytesRead,
    NULL
  );

  if (!readSuccess || bytesRead != bytesToRead)
  {
    HeapFree(GetProcessHeap(), 0, data);
    data = NULL;
    TRACE("File read failed, readSuccess=%d, bytesRead=%d, size=%d", readSuccess, bytesRead, bytesToRead);
  }

  return data;
}

void * TREFile::mallocAndRead(HANDLE handle, SIZE_T bytesToRead, SIZE_T decompressedBytes)
{
  void *compressedData = TREFile::mallocAndRead(handle, bytesToRead);
  void *uncompressedData = NULL;
  if (decompressedBytes != 0 && bytesToRead != decompressedBytes)
  {
    uncompressedData = HeapAlloc(GetProcessHeap(), 0, decompressedBytes);

    int ret;
    DWORD have = 0;
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);

    strm.avail_in = bytesToRead;
    strm.next_in = (Bytef *)compressedData;

    //do {
    strm.avail_out = decompressedBytes;
    strm.next_out = (Bytef *)uncompressedData;
    ret = inflate(&strm, Z_NO_FLUSH);
    //assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
    switch (ret) {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
      (void)inflateEnd(&strm);
      TRACE("inflate error: %d", ret);
      return 0;
    }
    have = decompressedBytes - strm.avail_out;
    //} while (strm.avail_out == 0);

    (void)inflateEnd(&strm);

    HeapFree(GetProcessHeap(), 0, compressedData);
    //return have;
  }
  else
  {
    uncompressedData = compressedData;
  }
  return uncompressedData;
}

std::vector<TREResource *> * TREFile::load(std::string filter)
{
  bool success = true;
  LPCSTR *resourceNames = NULL;
  TREInfoHeaderType **resources = NULL;

  std::vector<TREResource *> *result = new std::vector<TREResource *>;

  TREHeaderType *header = NULL;
  TREInfoHeaderType **infoHeaders = NULL;
  LPSTR names = NULL;
  BYTE *infoBuffer = NULL;
  BYTE *infoBufferOrig = NULL;

  if (success)
  {
    this->fileHandle = CreateFile(
      this->filePath,
      GENERIC_READ,
      FILE_SHARE_READ,
      NULL,
      OPEN_EXISTING,
      0,
      NULL
    );

    if (this->fileHandle == INVALID_HANDLE_VALUE)
    {
      success = false;
      TRACE("Could not open file [%s]", this->filePath);
    }
  }

  if (success)
  {
    /* Get file size, then reset to 0 */
    this->fileSize = SetFilePointer(this->fileHandle, 0, 0, FILE_END);
    SetFilePointer(this->fileHandle, 0, 0, FILE_BEGIN);

    if (this->fileSize < sizeof(TREHeaderType))
    {
      success = false;
      TRACE("File size is too small. Expected=%d, got=%d", sizeof(TREHeaderType), this->fileSize);
    }
  }

  if (success)
  {
    header = reinterpret_cast<TREHeaderType *>(TREFile::mallocAndRead(this->fileHandle, sizeof(TREHeaderType)));

    if (strncmp("EERT", header->fourcc, 4) != 0)
    {
      success = false;
      TRACE("Not a TRE file.");
    }
    if (strncmp("5000", header->version, 4) != 0)
    {
      success = false;
      TRACE("Not a TRE file that I understand.");
    }
    if (this->fileSize < header->infoOffset + header->infoCompressedSize + header->namesCompressedSize)
    {
      success = false;
      TRACE("File size is too small. infoOffset=0x%x, infoCompressedSize=%d, namesCompressedSize=%d, fileSize=%d", header->infoOffset, header->infoCompressedSize, header->namesCompressedSize, this->fileSize);
    }
    result->reserve(header->count);
    TRACE("count=%d, infoOffset=0x%x, infoCompressed=%d, infoCompressedSize=%d, namesCompressed=%d, namesCompressedSize=%d, namesSize=%d", header->count, header->infoOffset, header->namesCompressed, header->infoCompressedSize, header->namesCompressed, header->namesCompressedSize, header->namesSize);
  }

  if (success) {
    SetFilePointer(this->fileHandle, header->infoOffset, 0, FILE_BEGIN);
    DWORD bytesToRead = sizeof(TREInfoHeaderType) * header->count;

    if (header->infoCompressed)
    {
      bytesToRead = header->infoCompressedSize;
    }

    resources = reinterpret_cast<TREInfoHeaderType **>(HeapAlloc(GetProcessHeap(), 0, sizeof(resources) * header->count));
    infoBufferOrig = reinterpret_cast<BYTE *>(TREFile::mallocAndRead(this->fileHandle, bytesToRead, sizeof(TREInfoHeaderType) * header->count));
    infoBuffer = infoBufferOrig;
    for (DWORD32 i = 0; i < header->count; i++)
    {
      resources[i] = reinterpret_cast<TREInfoHeaderType *>(infoBuffer);
      TRACE("dataSize=%d, dataOffset=0x%x, dataCompressed=%d, dataCompressedSize=%d, nameOffset=0x%x", resources[i]->dataSize, resources[i]->dataOffset, resources[i]->dataCompressed, resources[i]->dataCompressedSize, resources[i]->nameOffset);
      infoBuffer += sizeof(TREInfoHeaderType);
    }
  }

  if (success) {
    DWORD bytesToRead = header->namesSize;

    if (header->namesCompressed)
    {
      bytesToRead = header->namesCompressedSize;
    }

    names = reinterpret_cast<LPSTR>(TREFile::mallocAndRead(this->fileHandle, bytesToRead, header->namesSize));
    resourceNames = reinterpret_cast<LPCSTR *>(HeapAlloc(GetProcessHeap(), 0, sizeof(resourceNames) * header->count));

    for (DWORD32 i = 0; i < header->count; i++)
    {
      DWORD32 offset = resources[i]->nameOffset;
      resourceNames[i] = &(names[offset]);
      TRACE("name: [%s]", resourceNames[i]);
    }

  }

  if (success)
  {
    for (DWORD32 i = 0; i < header->count; i++)
    {
      if (strncmp(resourceNames[i], filter.c_str(), strlen(filter.c_str())) == 0)
      {
        TREResource *resource = new TREResource(resources[i]->dataSize, resources[i]->dataOffset, resources[i]->dataCompressed, resources[i]->dataCompressedSize, this->filePath, resourceNames[i]);
        result->push_back(resource);
      }
    }
  }

  if (header)
  {
    HeapFree(GetProcessHeap(), 0, header);
  }
  if (resources)
  {
    HeapFree(GetProcessHeap(), 0, resources);
  }
  if (infoBufferOrig)
  {
    HeapFree(GetProcessHeap(), 0, infoBufferOrig);
  }
  if (names)
  {
    HeapFree(GetProcessHeap(), 0, names);
  }

  return result;
}
