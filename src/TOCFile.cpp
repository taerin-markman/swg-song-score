#include "stdafx.h"               /* Windows API             */
#include "..\zlib\include\zlib.h" /* Zlib Inflation          */
#include <stdio.h>

#include "Debug.h"
#include "TOCFile.h"

//#pragma comment(linker,"zdll.lib")

#pragma pack(push,1)


typedef struct
{
  char fourcc[4];
  char version[4];
  DWORD32 dunno1; // 0
  DWORD32 data_chunk_num;
  DWORD32 data_chunk_size;
  DWORD32 dunno2; // same as res_chunk_size
  DWORD32 res_chunk_size;
  DWORD32 file_chunk_num;
  DWORD32 file_chunk_size;
} SWGTOCHeaderType;

typedef struct
{
  WORD item_compressed;
  WORD file_num;
  WORD dunno2;
  WORD item_count;
  DWORD dunno3;
  DWORD item_address;
  DWORD item_size;
  DWORD compressed_item_size;
} SWGTOCDataChunkType;

typedef struct
{
  void    * next;
  unsigned int song_id;
  unsigned int inst_id;
  unsigned int flo_id;
  unsigned int file;
  DWORD  loc;
  DWORD  size;
} TOCDataType;
#pragma pack(pop)

TOCFile::TOCFile(LPCSTR filePath)
{
  this->resourceCount = 0;
  this->filePath = filePath;
  this->fileHandle = INVALID_HANDLE_VALUE;
}

void * TOCFile::mallocAndRead(HANDLE handle, SIZE_T bytesToRead)
{
  BOOL readSuccess = false;
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

std::vector<TREResource *> * TOCFile::load()
{
  BOOL success = true;
#ifdef _DEBUG
  HANDLE hOFile = NULL;
  char   outdata[512];
#endif /* _DEBUG */

  std::vector<TREResource *> *result = new std::vector<TREResource *>;

  SWGTOCHeaderType *header = NULL;

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

    if (this->fileSize < sizeof(SWGTOCHeaderType))
    {
      success = false;
      TRACE("File size is too small. Expected=%d, got=%d", sizeof(SWGTOCHeaderType), this->fileSize);
    }
  }

  if (success)
  {
    header = reinterpret_cast<SWGTOCHeaderType *>(TOCFile::mallocAndRead(this->fileHandle, sizeof(SWGTOCHeaderType)));

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
    if (this->fileSize < header->data_chunk_size + header->res_chunk_size + header->file_chunk_size)
    {
      success = false;
      TRACE("File size is too small. data_chunk_size=%d, res_chunk_size=%d, file_chunk_size=%d, fileSize=%d", header->data_chunk_size, header->res_chunk_size, header->file_chunk_size, this->fileSize);
    }
    //result->reserve(header->count);
    TRACE("data_chunk_num=%d, file_chunk_num=%d, data_chunk_size=%d, res_chunk_size=%d, file_chunk_size=%d", header->data_chunk_num, header->file_chunk_num, header->data_chunk_size, header->res_chunk_size, header->file_chunk_size);
  }

  if (success) {

    /*******************************
    get file list chunk
    *******************************/
    char *TOCFileListData = reinterpret_cast<char *>(HeapAlloc(GetProcessHeap(), 0, header->file_chunk_size));
    char **TOCFileList = reinterpret_cast<char **>(HeapAlloc(GetProcessHeap(), 0, sizeof(char *) * header->file_chunk_num));
    DWORD BytesRead = 0;
    char * a = TOCFileListData;

    BOOL b_r = ReadFile(
      this->fileHandle,
      a,
      header->file_chunk_size,
      &BytesRead,
      NULL
    );

    unsigned int FileNum = 0;
    do
    {
      TOCFileList[FileNum] = a;
      while (*a++ != 0);
      FileNum++;
    } while (FileNum < header->file_chunk_num);


    /*******************************
    get data list chunk
    *******************************/
    SWGTOCDataChunkType * toc_data_list = reinterpret_cast<SWGTOCDataChunkType *>(HeapAlloc(GetProcessHeap(), 0, sizeof(SWGTOCDataChunkType) *header->data_chunk_num));

    unsigned long NumEntry = 0;

    while (NumEntry < header->data_chunk_num)
    {
      b_r = ReadFile(
        this->fileHandle,
        &toc_data_list[NumEntry],
        sizeof(SWGTOCDataChunkType),
        &BytesRead,
        NULL
      );

      NumEntry++;
    }

    /*******************************
    get resource list chunk
    *******************************/
    char * toc_res_list = reinterpret_cast<char *>(HeapAlloc(GetProcessHeap(), 0, header->res_chunk_size));
    char * b = toc_res_list;
    char ** res_list = reinterpret_cast<char **>(HeapAlloc(GetProcessHeap(), 0, sizeof(char *) * header->data_chunk_num));

    b_r = ReadFile(
      this->fileHandle,
      toc_res_list,
      header->res_chunk_size,
      &BytesRead,
      NULL
    );

    unsigned int ResNum = 0;
    do
    {
      res_list[ResNum] = b;
      while (*b++ != 0);
      ResNum++;
    } while (ResNum < header->data_chunk_num);

    /*******************************
    done, now write it out
    *******************************/
    for (unsigned int x = 0; x < header->data_chunk_num; x++)
    {
      if (strncmp(res_list[x], "player_music/sample/song", 24) == 0)
      {
#ifdef _DEBUG
        sprintf(outdata, "0x%x,,0x%x,0x%x,%s,0x%x,,%s,0x%x,0x%x,0x%x,0x%x\r\n\0", x, toc_data_list[x].item_address, toc_data_list[x].item_size, TOCFileList[toc_data_list[x].file_num], toc_data_list[x].item_count, res_list[x], toc_data_list[x].item_compressed, toc_data_list[x].dunno2, toc_data_list[x].dunno3, toc_data_list[x].compressed_item_size);

        b_r = WriteFile(
          hOFile,
          outdata,
          strlen(outdata),
          &BytesRead,
          NULL
        );
#endif /* _DEBUG */

            TREResource *resource = new TREResource(toc_data_list[x].item_size, toc_data_list[x].item_address, toc_data_list[x].item_compressed, toc_data_list[x].compressed_item_size, TOCFileList[toc_data_list[x].file_num], res_list[x]);
            result->push_back(resource);

      }
    }

  }

  //if (success)
  //{
  //  for (int i = 0; i < header->count; i++)
  //  {
  //    TREResource *resource = new TREResource(resources[i]->dataSize, resources[i]->dataOffset, resources[i]->dataCompressed, resources[i]->dataCompressedSize, this->filePath, resourceNames[i]);
  //    result->push_back(resource);
  //  }
  //}

  //if (header)
  //{
  //  HeapFree(GetProcessHeap(), 0, header);
  //}
  //if (resources)
  //{
  //  HeapFree(GetProcessHeap(), 0, resources);
  //}
  //if (infoBufferOrig)
  //{
  //  HeapFree(GetProcessHeap(), 0, infoBufferOrig);
  //}
  //if (names)
  //{
  //  HeapFree(GetProcessHeap(), 0, names);
  //}

  return result;
}


#if 0
bool LoadTOCData()
{
  HANDLE hFile = NULL;
#ifdef _DEBUG
  HANDLE hOFile = NULL;
  char   outdata[512];
#endif /* _DEBUG */
  BOOL  b_r = TRUE;
  DWORD BytesRead;
  DWORD FileSize = 0;
  SWGTOCHeaderType    toc_header;
  bool retval = true;

  hFile = CreateFile(
    "sku0_client.toc",
    GENERIC_READ,
    FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    0,
    NULL
  );

#ifdef _DEBUG
  hOFile = CreateFile(
    "sku0_client.csv",
    GENERIC_WRITE,
    FILE_SHARE_WRITE,
    NULL,
    CREATE_ALWAYS,
    0,
    NULL
  );
#endif /* _DEBUG */

  FileSize = SetFilePointer(hFile, 0, 0, FILE_END);
  SetFilePointer(hFile, 0, 0, FILE_BEGIN);

  b_r = ReadFile(
    hFile,
    &toc_header,
    sizeof(toc_header),
    &BytesRead,
    NULL
  );
  if (!b_r)
  {
    return false;
  }

  /*******************************
  get file list chunk
  *******************************/
  TOCFileListData = reinterpret_cast<char *>(HeapAlloc(GetProcessHeap(), 0, toc_header.file_chunk_size));
  TOCFileList = reinterpret_cast<char **>(HeapAlloc(GetProcessHeap(), 0, sizeof(char *) * toc_header.file_chunk_num));

  char * a = TOCFileListData;

  b_r = ReadFile(
    hFile,
    a,
    toc_header.file_chunk_size,
    &BytesRead,
    NULL
  );

  unsigned int FileNum = 0;
  do
  {
    TOCFileList[FileNum] = a;
    while (*a++ != 0);
    FileNum++;
  } while (FileNum < toc_header.file_chunk_num);


  /*******************************
  get data list chunk
  *******************************/
  SWGTOCDataChunkType * toc_data_list = reinterpret_cast<SWGTOCDataChunkType *>(HeapAlloc(GetProcessHeap(), 0, sizeof(SWGTOCDataChunkType) * toc_header.data_chunk_num));

  unsigned long NumEntry = 0;

  while (NumEntry < toc_header.data_chunk_num)
  {
    b_r = ReadFile(
      hFile,
      &toc_data_list[NumEntry],
      sizeof(SWGTOCDataChunkType),
      &BytesRead,
      NULL
    );

    NumEntry++;
  }

  /*******************************
  get resource list chunk
  *******************************/
  char * toc_res_list = reinterpret_cast<char *>(HeapAlloc(GetProcessHeap(), 0, toc_header.res_chunk_size));
  char * b = toc_res_list;
  char ** res_list = reinterpret_cast<char **>(HeapAlloc(GetProcessHeap(), 0, sizeof(char *) * toc_header.data_chunk_num));

  b_r = ReadFile(
    hFile,
    toc_res_list,
    toc_header.res_chunk_size,
    &BytesRead,
    NULL
  );

  unsigned int ResNum = 0;
  do
  {
    res_list[ResNum] = b;
    while (*b++ != 0);
    ResNum++;
  } while (ResNum < toc_header.data_chunk_num);


  /*******************************
  done, now write it out
  *******************************/
  unsigned int NumSamples = 0;
  bool * res_list_usable = reinterpret_cast<bool *>(HeapAlloc(GetProcessHeap(), 0, sizeof(bool) * toc_header.data_chunk_num));

  for (unsigned int x = 0; x < toc_header.data_chunk_num; x++)
  {
    res_list_usable[x] = false;
    if (strncmp(res_list[x], "player_music/sample/song", 24) == 0)
    {
#ifdef _DEBUG
      sprintf(outdata, "0x%x,,0x%x,0x%x,%s,0x%x,,%s\r\n\0", x, toc_data_list[x].item_address, toc_data_list[x].item_size, TOCFileList[toc_data_list[x].file_num], toc_data_list[x].item_count, res_list[x]);

      b_r = WriteFile(
        hOFile,
        outdata,
        strlen(outdata),
        &BytesRead,
        NULL
      );
#endif /* _DEBUG */

      res_list_usable[x] = true;
      NumSamples++;
    }
  }

  unsigned int SongNum = 1;
  bool bFileVerified[100] = { false };
  for (x = 0; x < toc_header.data_chunk_num; x++)
  {
    if (res_list_usable[x])
    {
      char filepath[256];

      unsigned int        song_id = 0;
      FlourishSoundType   flo_id = FLOURISH_SOUND_MAX;
      InstrumentSoundType inst_id = INSTRUMENT_SOUND_MAX;
      char *              res_loc = res_list[x] + 24;
      HANDLE              hTestFile;

      /*******************************
      verify tre file exists
      *******************************/
      if (!bFileVerified[toc_data_list[x].file_num])
      {
        hTestFile = CreateFile(
          TOCFileList[toc_data_list[x].file_num],
          GENERIC_READ,
          FILE_SHARE_READ,
          NULL,
          OPEN_EXISTING,
          0,
          NULL
        );

        if (hTestFile == INVALID_HANDLE_VALUE)
        {
          sprintf(filepath, "%s%s", "..\\", TOCFileList[toc_data_list[x].file_num]);

          hTestFile = CreateFile(
            filepath,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
          );

          if (hTestFile == INVALID_HANDLE_VALUE)
          {
            sprintf(filepath, "%s%s", "testcenter\\", TOCFileList[toc_data_list[x].file_num]);

            hTestFile = CreateFile(
              filepath,
              GENERIC_READ,
              FILE_SHARE_READ,
              NULL,
              OPEN_EXISTING,
              0,
              NULL
            );

            if (hTestFile == INVALID_HANDLE_VALUE)
            {

              CloseHandle(hTestFile);
              continue;
            }
          }
        }
        CloseHandle(hTestFile);

        bFileVerified[toc_data_list[x].file_num] = true;
      }

      /*******************************
      song
      *******************************/
      if (1 == sscanf(res_loc, "%02d", &song_id))
      {
        res_loc += 3;

        /*******************************
        instrument
        *******************************/
        if (0 == strncmp("mand", res_loc, 4))
        {
          inst_id = INSTRUMENT_SOUND_MANDOVIOL;
          res_loc += 5;
        }
        else if (0 == strncmp("khorn", res_loc, 5))
        {
          inst_id = INSTRUMENT_SOUND_CHIDINKALU;
          res_loc += 6;
        }
        else if (0 == strncmp("shorn", res_loc, 5))
        {
          inst_id = INSTRUMENT_SOUND_TRAZ;
          res_loc += 6;
        }
        else if (0 == strncmp("nlrg", res_loc, 4))
        {
          inst_id = INSTRUMENT_SOUND_NALARGON;
          res_loc += 5;
        }
        else if (0 == strncmp("drum", res_loc, 4))
        {
          inst_id = INSTRUMENT_SOUND_BANDFILL;
          res_loc += 5;
        }
        else if (0 == strncmp("xantha", res_loc, 6))
        {
          inst_id = INSTRUMENT_SOUND_XANTHA;
          res_loc += 7;
        }
        else
        {
          TRACE("Bad instrument parse.", 0, 0, 0);
          retval = false;
          break;
        }

        /*******************************
        flourish
        *******************************/
        unsigned int t_flo_id = 0;
        if (0 == strncmp("flourish", res_loc, 8))
        {
          res_loc += 8;
          if (1 != sscanf(res_loc, "%02d", &t_flo_id))
          {
            TRACE("Bad flourish parse.", 0, 0, 0);
            retval = false;
            break;
          }
          flo_id = (FlourishSoundType)(t_flo_id - 1);
        }
        else if (0 == strncmp("intro", res_loc, 5))
        {
          flo_id = FLOURISH_SOUND_INTRO;
        }
        else if (0 == strncmp("outro", res_loc, 5))
        {
          flo_id = FLOURISH_SOUND_OUTRO;
        }
        else if (0 == strncmp("main", res_loc, 4))
        {
          flo_id = FLOURISH_SOUND_IDLE;
        }
        else if (1 == sscanf(res_loc, "%01d_", &t_flo_id))
        {
          flo_id = (FlourishSoundType)(t_flo_id - 1);
        }
        else
        {
          TRACE("Bad flourish parse.", 0, 0, 0);
          retval = false;
          break;
        }


        /*******************************
        package result
        *******************************/
        RegisterFlourishData(
          song_id,
          inst_id,
          flo_id,
          toc_data_list[x].file_num,
          toc_data_list[x].item_address,
          toc_data_list[x].item_size
        );
        /*
        if (TOCDataSongMap[song_id] == 0)
        {
        TOCDataSongMap[song_id] = SongNum;
        SongNum++;
        }
        TOCData[TOCDataSongMap[song_id]][inst_id][flo_id].ena  = true;
        TOCData[TOCDataSongMap[song_id]][inst_id][flo_id].file = toc_data_list[x].file_num;
        TOCData[TOCDataSongMap[song_id]][inst_id][flo_id].loc  = toc_data_list[x].item_address;
        TOCData[TOCDataSongMap[song_id]][inst_id][flo_id].size = toc_data_list[x].item_size;
        */
      }
      else
      {
        TRACE("Could not scan resource item.", 0, 0, 0);
        retval = false;
        break;
      }
    }
  }

  HeapFree(GetProcessHeap(), 0, toc_data_list);

  HeapFree(GetProcessHeap(), 0, toc_res_list);
  HeapFree(GetProcessHeap(), 0, res_list);

  HeapFree(GetProcessHeap(), 0, res_list_usable);

  CloseHandle(hFile);
#ifdef _DEBUG
  CloseHandle(hOFile);
#endif /* _DEBUG */
  return retval;
}

#endif
