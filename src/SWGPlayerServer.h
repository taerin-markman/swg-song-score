#pragma once
#include "stdafx.h"

#include "Song.h"

#define MAX_FLOURISH_BUFFERS 6

#define SWGC_STATUS (WM_USER+1)
#define SWGC_ADD_SONG (SWGC_STATUS+1)

typedef enum {
  SWG_PLAYER_STOP_GRACEFUL,
  SWG_PLAYER_STOP_END_OF_PHRASE,
  SWG_PLAYER_STOP_NOW,
  SWG_PLAYER_STOP_MAX
} SWGPlayerStopType;

typedef struct {
  FlourishSoundType SoundList[NUM_INSTRUMENTS];
} SoundListType;

typedef struct {
  SoundListType Buffer[MAX_FLOURISH_BUFFERS];
  SoundListType BandBuffer;
  SoundListType RandBuffer;
  bool          BandFlo;
  int           CurrFloIndex[NUM_INSTRUMENTS];
  int           UsedFloBuffs[NUM_INSTRUMENTS];
  int           CurrInsertLoc[NUM_INSTRUMENTS];
} FlourishBufferType;

typedef enum {
  SWG_PLAYER_CMD_PLAY,
  SWG_PLAYER_CMD_STOP,
  SWG_PLAYER_CMD_FLOURISH,
  SWG_PLAYER_CMD_CHANGE,
  SWG_PLAYER_CMD_ENABLE_INSTRUMENT,
  SWG_PLAYER_CMD_BAND_FLOURISH,
  SWG_PLAYER_CMD_SET_RANDOM_FLOURISH,
  SWG_PLAYER_CMD_SET_DIALOG_HANDLE,
  SWG_PLAYER_CMD_MAX
} SWGPlayerCmdType;

typedef struct {
  void           * Next;
  SWGPlayerCmdType Command;
  DWORD            Payload[2];
} SWGPlayerCmdQueueType;

class SWGPlayerServer
{
protected:
  bool HardStop, SoftStop, SofterStop;
  //DWORD player_hWnd;
  HWND ClientHWND;
  HWND StatusHWND;
  HANDLE ThreadHandle;
  SWGPlayerCmdQueueType * CmdQueue = NULL;
  HANDLE hMutex;
  ResourceRegistry *Registry;

  Song *CurrentSong = NULL;

  bool RandomizerEnabled = false;

  LPDIRECTSOUND8 DSound;
  LPDIRECTSOUNDBUFFER   DSBuffer[NUM_INSTRUMENTS] = { NULL };
  DWORD BuffStatus = 0;
  DWORD InstrumentBitMask = 0;
  bool EnabledInstrument[NUM_INSTRUMENTS] = { false };

  SoundListType CurrentFlourish = {
    FLOURISH_SOUND_IDLE,FLOURISH_SOUND_IDLE,FLOURISH_SOUND_IDLE,FLOURISH_SOUND_IDLE,FLOURISH_SOUND_IDLE
  };

  void PlayNextFlourish(SoundListType PhraseList);
  bool CheckPhrasePlaybackStatus();
  bool CreateDSoundObject();
  bool ReleaseSoundBuffers();
  int GetNextFlourish(SoundListType * DestBuff, FlourishBufferType * FloBuff);
  int InsertBandFlourish(FlourishBufferType * FloBuff, FlourishSoundType Flourish, bool Final);
  int InsertFlourish(FlourishBufferType * FloBuff, InstrumentSoundType Instrument, FlourishSoundType Flourish, bool FlushQueue = false);
  void InitFlourish(FlourishBufferType * FloBuff);
  void GenerateRandomSet(FlourishBufferType * FloBuff);
  void SendPlaylistUpdate(FlourishBufferType * FloBuff, SoundListType * NowPlaying);

  bool GetCommand(SWGPlayerCmdType * Command, DWORD * Payload1, DWORD * Payload2);
  static DWORD WINAPI SWGPlayerProc(LPVOID lpParameter);

public:
  SWGPlayerServer(HWND hWnd, ResourceRegistry *registry);
  bool SendCommand(SWGPlayerCmdType Command, DWORD Payload1, DWORD Payload2);
};
