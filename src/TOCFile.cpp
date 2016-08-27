#include "stdafx.h"               /* Windows API             */
#include <stdio.h>
#include "Debug.h"
#include "TOCFile.h"

#pragma pack(push,1)
typedef struct
{
  char fourcc[4];
  char version[4];
  DWORD32 dunno1; // 0
  DWORD32 dataCount;
  DWORD32 dataSize;
  DWORD32 dunno2; // same as res_chunk_size
  DWORD32 namesSize;
  DWORD32 filesCount;
  DWORD32 filesSize;
} TOCHeaderType;

typedef struct
{
  WORD infoCompressed;
  WORD fileIndex;
  WORD dunno2;
  WORD infoCount;
  DWORD32 dunno3;
  DWORD32 infoOffset;
  DWORD32 infoSize;
  DWORD32 infoCompressedSize;
} TOCInfoHeaderType;
#pragma pack(pop)

TOCFile::TOCFile(LPCSTR filePath)
{
  this->resourceCount = 0;
  this->filePath = filePath;
  this->fileHandle = INVALID_HANDLE_VALUE;
}

void * TOCFile::mallocAndRead(HANDLE handle, SIZE_T bytesToRead)
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

std::vector<TREResource *> * TOCFile::load(std::string filter)
{
  bool success = true;
#ifdef _DEBUG
  HANDLE hOFile = NULL;
  char   outdata[512];
#endif /* _DEBUG */
  char *TOCFileListData = NULL;
  char **TOCFileList = NULL;
  char *toc_res_list = NULL;
  char **res_list = NULL;

  std::vector<TREResource *> *result = new std::vector<TREResource *>;

  TOCHeaderType *header = NULL;

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
#ifdef _DEBUG
    char debugPath[512] = { 0 };
    sprintf_s(debugPath, 512, "%s.csv", this->filePath);
    hOFile = CreateFile(
      debugPath,
      GENERIC_WRITE,
      FILE_SHARE_WRITE,
      NULL,
      CREATE_ALWAYS,
      0,
      NULL
    );
#endif /* _DEBUG */
  }

  if (success)
  {
    /* Get file size, then reset to 0 */
    this->fileSize = SetFilePointer(this->fileHandle, 0, 0, FILE_END);
    SetFilePointer(this->fileHandle, 0, 0, FILE_BEGIN);

    if (this->fileSize < sizeof(TOCHeaderType))
    {
      success = false;
      TRACE("File size is too small. Expected=%d, got=%d", sizeof(TOCHeaderType), this->fileSize);
    }
  }

  if (success)
  {
    header = reinterpret_cast<TOCHeaderType *>(TOCFile::mallocAndRead(this->fileHandle, sizeof(TOCHeaderType)));

    if (strncmp(" COT", header->fourcc, 4) != 0)
    {
      success = false;
      TRACE("Not a TOC file.");
    }
    if (strncmp("1000", header->version, 4) != 0)
    {
      success = false;
      TRACE("Not a TOC file that I understand.");
    }
    if (this->fileSize < header->dataSize + header->namesSize + header->filesSize)
    {
      success = false;
      TRACE("File size is too small. dataSize=%d, namesSize=%d, filesSize=%d, fileSize=%d", header->dataSize, header->namesSize, header->filesSize, this->fileSize);
    }
    //result->reserve(header->count);
    TRACE("dataCount=%d, filesCount=%d, dataSize=%d, namesSize=%d, filesSize=%d", header->dataCount, header->filesCount, header->dataSize, header->namesSize, header->filesSize);
  }

  if (success) {

    /*******************************
    get file list chunk
    *******************************/
    TOCFileListData = reinterpret_cast<char *>(HeapAlloc(GetProcessHeap(), 0, header->filesSize));
    TOCFileList = reinterpret_cast<char **>(HeapAlloc(GetProcessHeap(), 0, sizeof(char *) * header->filesCount));
    DWORD BytesRead = 0;
    char * a = TOCFileListData;

    BOOL b_r = ReadFile(
      this->fileHandle,
      a,
      header->filesSize,
      &BytesRead,
      NULL
    );

    unsigned int FileNum = 0;
    do
    {
      TOCFileList[FileNum] = a;
      while (*a++ != 0);
      FileNum++;
    } while (FileNum < header->filesCount);


    /*******************************
    get data list chunk
    *******************************/
    TOCInfoHeaderType * TOCDataList = reinterpret_cast<TOCInfoHeaderType *>(HeapAlloc(GetProcessHeap(), 0, sizeof(TOCInfoHeaderType) *header->dataCount));

    unsigned long NumEntry = 0;

    while (NumEntry < header->dataCount)
    {
      b_r = ReadFile(
        this->fileHandle,
        &TOCDataList[NumEntry],
        sizeof(TOCInfoHeaderType),
        &BytesRead,
        NULL
      );

      NumEntry++;
    }

    /*******************************
    get resource list chunk
    *******************************/
    toc_res_list = reinterpret_cast<char *>(HeapAlloc(GetProcessHeap(), 0, header->namesSize));
    char * b = toc_res_list;
    res_list = reinterpret_cast<char **>(HeapAlloc(GetProcessHeap(), 0, sizeof(char *) * header->dataCount));

    b_r = ReadFile(
      this->fileHandle,
      toc_res_list,
      header->namesSize,
      &BytesRead,
      NULL
    );

    unsigned int ResNum = 0;
    do
    {
      res_list[ResNum] = b;
      while (*b++ != 0);
      ResNum++;
    } while (ResNum < header->dataCount);

    /*******************************
    done, now write it out
    *******************************/
    for (unsigned int x = 0; x < header->dataCount; x++)
    {
      if (strncmp(res_list[x], "player_music/sample/song", 24) == 0)
      {
#ifdef _DEBUG
        sprintf_s(outdata, 512, "0x%x,,0x%x,0x%x,%s,0x%x,,%s,0x%x,0x%x,0x%x,0x%x\r\n\0", x, TOCDataList[x].infoOffset, TOCDataList[x].infoSize, TOCFileList[TOCDataList[x].fileIndex], TOCDataList[x].infoCount, res_list[x], TOCDataList[x].infoCompressed, TOCDataList[x].dunno2, TOCDataList[x].dunno3, TOCDataList[x].infoCompressedSize);

        b_r = WriteFile(
          hOFile,
          outdata,
          strlen(outdata),
          &BytesRead,
          NULL
        );
#endif /* _DEBUG */
        if (strncmp(res_list[x], filter.c_str(), strlen(filter.c_str())) == 0)
        {
          TREResource *resource = new TREResource(TOCDataList[x].infoSize, TOCDataList[x].infoOffset, TOCDataList[x].infoCompressed, TOCDataList[x].infoCompressedSize, TOCFileList[TOCDataList[x].fileIndex], res_list[x]);
          result->push_back(resource);
        }

      }
    }

  }

  if (header)
  {
    HeapFree(GetProcessHeap(), 0, header);
  }
  if (TOCFileListData)
  {
    HeapFree(GetProcessHeap(), 0, TOCFileListData);
  }
  if (TOCFileList)
  {
    HeapFree(GetProcessHeap(), 0, TOCFileList);
  }
  if (toc_res_list)
  {
    HeapFree(GetProcessHeap(), 0, toc_res_list);
  }
  if (res_list)
  {
    HeapFree(GetProcessHeap(), 0, res_list);
  }

  return result;
}
