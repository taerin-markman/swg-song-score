#include "stdafx.h"
#include "Flourish.h"
#include "Debug.h"
#include "mp3dec.h"


#define WAV_HEADER_SIZE 36

typedef struct {
  char  ChunkID[4];
  DWORD ChunkSize;
} RIFFChunkType;

typedef struct {
  RIFFChunkType ChunkHead;
  WORD          CompressionCode;
  WORD          NumChannels;
  DWORD         SampleRate;
  DWORD         AvgBytesPerSecond;
  WORD          BlockAlign;
  WORD          BitsPerSample;
  WORD          ExtraFormatBytes;
} RIFFfmtType;

typedef struct {
  RIFFChunkType RIFFChunk;
  char          RIFFTypeID[4];
  RIFFfmtType   fmtChunk;
} RIFFHeaderType;

std::string Flourish::generateResourceNameVariant1(FlourishSoundType type, std::string prefix)
{
  std::string newResourceName;
  switch (type)
  {
  case FLOURISH_SOUND_1:
    newResourceName = prefix + "_flourish01_lp.wav";
    break;
  case FLOURISH_SOUND_2:
    newResourceName = prefix + "_flourish02_lp.wav";
    break;
  case FLOURISH_SOUND_3:
    newResourceName = prefix + "_flourish03_lp.wav";
    break;
  case FLOURISH_SOUND_4:
    newResourceName = prefix + "_flourish04_lp.wav";
    break;
  case FLOURISH_SOUND_5:
    newResourceName = prefix + "_flourish05_lp.wav";
    break;
  case FLOURISH_SOUND_6:
    newResourceName = prefix + "_flourish06_lp.wav";
    break;
  case FLOURISH_SOUND_7:
    newResourceName = prefix + "_flourish07_lp.wav";
    break;
  case FLOURISH_SOUND_8:
    newResourceName = prefix + "_flourish08_lp.wav";
    break;
  case FLOURISH_SOUND_INTRO:
    newResourceName = prefix + "_intro.wav";
    break;
  case FLOURISH_SOUND_IDLE:
    newResourceName = prefix + "_main_lp.wav";
    break;
  case FLOURISH_SOUND_OUTRO:
    newResourceName = prefix + "_outro.wav";
    break;
  default:
    TRACE("Bad flourish value: %d", type);
    break;
  }
  return newResourceName;
}

std::string Flourish::generateResourceNameVariant2(FlourishSoundType type, std::string prefix)
{
  std::string newResourceName;
  switch (type)
  {
  case FLOURISH_SOUND_1:
    newResourceName = prefix + "_1_lp.wav";
    break;
  case FLOURISH_SOUND_2:
    newResourceName = prefix + "_2_lp.wav";
    break;
  case FLOURISH_SOUND_3:
    newResourceName = prefix + "_3_lp.wav";
    break;
  case FLOURISH_SOUND_4:
    newResourceName = prefix + "_4_lp.wav";
    break;
  case FLOURISH_SOUND_5:
    newResourceName = prefix + "_5_lp.wav";
    break;
  case FLOURISH_SOUND_6:
    newResourceName = prefix + "_6_lp.wav";
    break;
  case FLOURISH_SOUND_7:
    newResourceName = prefix + "_7_lp.wav";
    break;
  case FLOURISH_SOUND_8:
    newResourceName = prefix + "_8_lp.wav";
    break;
  case FLOURISH_SOUND_INTRO:
    newResourceName = prefix + "_intro.wav";
    break;
  case FLOURISH_SOUND_IDLE:
    newResourceName = prefix + "_main_lp.wav";
    break;
  case FLOURISH_SOUND_OUTRO:
    newResourceName = prefix + "_outro.wav";
    break;
  default:
    TRACE("Bad flourish value: %d", type);
    break;
  }
  return newResourceName;
}


Flourish::Flourish(FlourishSoundType type, ResourceRegistry *registry, std::string resourcePrefix)
{
  this->type = type;

  this->resourceName = this->generateResourceNameVariant1(type, resourcePrefix);
  this->resource = registry->resource(this->resourceName);
  if (!this->resource)
  {
    this->resourceName = this->generateResourceNameVariant2(type, resourcePrefix);
    this->resource = registry->resource(this->resourceName);
    if (!this->resource)
    {
      TRACE("Could not find resource for [%s]", this->resourceName.c_str());
    }
  }
  this->data = NULL;

  this->wavFormat = reinterpret_cast<WAVEFORMATEX *>(HeapAlloc(GetProcessHeap(), 0, sizeof(WAVEFORMATEX)));
  this->dsBufferDesc = reinterpret_cast<DSBUFFERDESC *>(HeapAlloc(GetProcessHeap(), 0, sizeof(DSBUFFERDESC)));
  this->dsBufferDesc->lpwfxFormat = NULL;
  this->dsBufferDesc->dwBufferBytes = 0;
  this->dsBufferDesc->dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRL3D;
  this->dsBufferDesc->dwReserved = 0;
  this->dsBufferDesc->dwSize = sizeof(DSBUFFERDESC);
  this->dsBufferDesc->guid3DAlgorithm = DS3DALG_DEFAULT;

}

Flourish::~Flourish()
{
  if (this->wavFormat)
  {
    HeapFree(GetProcessHeap(), 0, this->wavFormat);
  }
  if (this->dsBufferDesc)
  {
    HeapFree(GetProcessHeap(), 0, this->dsBufferDesc);
  }
  if (this->data)
  {
    HeapFree(GetProcessHeap(), 0, this->data);
  }
}

bool Flourish::prepare()
{
  bool success = false;

  if (!this->resource) { return success; }

  void *fileData = NULL;
  DWORD bytesRead = this->resource->load(&fileData);

  boolean bFileCompressed = false;
  RIFFHeaderType * RIFFHeader;
  RIFFChunkType  * RIFFData;
  mp3header_format_type * mp3header;

  RIFFHeader = (RIFFHeaderType *)fileData;
  mp3header = (mp3header_format_type *)fileData;

  if (mp3header->sync1 == 0xff
    && mp3header->sync2 == 0x7
    && mp3header->layer == 1)
  {
    int osize = 0;
    char * buff = NULL;

    convertMP3(
      (char *)fileData,
      bytesRead,
      this->wavFormat,
      &osize,
      &buff
    );

    this->dsBufferDesc->lpwfxFormat = this->wavFormat;
    this->dsBufferDesc->dwBufferBytes = osize;
    this->dsBufferDesc->dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRL3D;
    this->dsBufferDesc->dwReserved = 0;
    this->dsBufferDesc->dwSize = sizeof(DSBUFFERDESC);
    this->dsBufferDesc->guid3DAlgorithm = DS3DALG_DEFAULT;

    this->data = buff;
    TRACE("fbuff = %x", (unsigned int)buff);
  }
  else
  {

    if (strncmp(RIFFHeader->RIFFChunk.ChunkID, "RIFF", 4) != 0
      || strncmp(RIFFHeader->RIFFTypeID, "WAVE", 4) != 0
      || strncmp(RIFFHeader->fmtChunk.ChunkHead.ChunkID, "fmt ", 4) != 0
      )
    {
      RIFFHeader = (RIFFHeaderType *)fileData;

      if (strncmp(RIFFHeader->RIFFChunk.ChunkID, "RIFF", 4) != 0
        || strncmp(RIFFHeader->RIFFTypeID, "WAVE", 4) != 0
        || strncmp(RIFFHeader->fmtChunk.ChunkHead.ChunkID, "fmt ", 4) != 0
        )
      {
        HeapFree(GetProcessHeap(), 0, fileData);
        TRACE("DSOUND: ERROR: Not normal RIFF!");
        return false;
      }
    }

    this->wavFormat->cbSize = 0;
    this->wavFormat->nChannels = RIFFHeader->fmtChunk.NumChannels;
    this->wavFormat->nSamplesPerSec = RIFFHeader->fmtChunk.SampleRate;
    this->wavFormat->wBitsPerSample = RIFFHeader->fmtChunk.BitsPerSample;
    this->wavFormat->wFormatTag = WAVE_FORMAT_PCM;
    this->wavFormat->nBlockAlign =
      (this->wavFormat->nChannels * this->wavFormat->wBitsPerSample) / 8;
    this->wavFormat->nAvgBytesPerSec =
      this->wavFormat->nSamplesPerSec * this->wavFormat->nBlockAlign;

    /* Either skip past ExtraFormatBytes, if it exists, or
    ** back up to consume the 4 bytes it took to express its
    ** size if it doesn't exist.
    */
    if (RIFFHeader->fmtChunk.ChunkHead.ChunkSize != 16)
    {
      RIFFData = (RIFFChunkType *)((char *)RIFFHeader + RIFFHeader->fmtChunk.ExtraFormatBytes + WAV_HEADER_SIZE + 2);
    }
    else
    {
      RIFFData = (RIFFChunkType *)((char *)RIFFHeader + WAV_HEADER_SIZE);
    }

    if (strncmp(RIFFData->ChunkID, "data", 4) != 0)
    {
      TRACE("DSOUND: ERROR: Can't find data chunk!");
      return false;
    }

    this->dsBufferDesc->lpwfxFormat = this->wavFormat;
    this->dsBufferDesc->dwBufferBytes = RIFFData->ChunkSize;
    this->dsBufferDesc->dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRL3D;
    this->dsBufferDesc->dwReserved = 0;
    this->dsBufferDesc->dwSize = sizeof(DSBUFFERDESC);
    this->dsBufferDesc->guid3DAlgorithm = DS3DALG_DEFAULT;

    this->data = reinterpret_cast<char*>(HeapAlloc(GetProcessHeap(), 0, RIFFData->ChunkSize));
    if (this->data == NULL)
    {
      TRACE("DSOUND: ERROR: Couldn't allocate memory: %d\n", RIFFData->ChunkSize);
          HeapFree(GetProcessHeap(), 0, this->data);
      return false;
    }

    memcpy(this->data, (char *)RIFFData + sizeof(RIFFChunkType), RIFFData->ChunkSize);

  }

  HeapFree(GetProcessHeap(), 0, RIFFHeader);

  return success;
}

DSBUFFERDESC * Flourish::getDSBufferDesc()
{
  return this->dsBufferDesc;
}

void *Flourish::getData()
{
  return this->data;
}
