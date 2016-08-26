#include "stdafx.h"               /* Windows API             */
#include <dsound.h>
#include <stdio.h>
#include <time.h>

#include "SWG Player.h"
#include "Debug.h"
#include "ResourceRegistry.h"

ResourceRegistry *registry = new ResourceRegistry();

/*
typedef struct
{
  unsigned __int32 sync:11;
  unsigned __int32 version:2;
  unsigned __int32 layer:2;
  unsigned __int32 protection:1;
  unsigned __int32 bitrate_idx:4;
  unsigned __int32 sampling_rate:2;
  unsigned __int32 padding_bit:1;
  unsigned __int32 private_bit:1;
  unsigned __int32 channel_mode:2;
  unsigned __int32 mode_extension:2;
  unsigned __int32 copyright:1;
  unsigned __int32 original:1;
  unsigned __int32 emphasis:2;
} mp3header_format_type;
*/

LPDIRECTSOUND8 DSound;
LPDIRECTSOUNDBUFFER   DSBuffer[NUM_INSTRUMENTS];
DWORD          FileSize = 0;
DWORD BuffStatus = 0;
DWORD InstrumentBitMask = 0;
SongSoundType SongNum;

static int GetNextFlourish(SoundListType * DestBuff, FlourishBufferType * FloBuff);
static int InsertFlourish(FlourishBufferType * FloBuff, InstrumentSoundType Instrument, FlourishSoundType Flourish, bool FlushQueue = false);
static void InitFlourish(FlourishBufferType * FloBuff);
static int InsertBandFlourish(FlourishBufferType * FloBuff, FlourishSoundType Flourish, bool Final);
bool ReleaseSoundBuffers(void);
bool CreateDSoundObject(void);
void PlayNextFlourish(SoundListType PhraseList);
bool CheckPhrasePlaybackStatus(void);

static bool EnabledInstrument[NUM_INSTRUMENTS] = {
    false, false, false, false, false, false
};


HWND client_hDlg = 0;

bool RandomFlourish = false;
DWORD player_hWnd;
HWND client_hWnd;
HANDLE ThreadHandle;
PhraseListParamsType PhraseListParams;
DWORD WINAPI SWGPlayerProc(LPVOID lpParameter);

SoundListType CurrentFlourish = {
  FLOURISH_SOUND_IDLE,FLOURISH_SOUND_IDLE,FLOURISH_SOUND_IDLE,FLOURISH_SOUND_IDLE,FLOURISH_SOUND_IDLE
};

SoundListType DefaultFlourish = {
  FLOURISH_SOUND_IDLE,FLOURISH_SOUND_IDLE,FLOURISH_SOUND_IDLE,FLOURISH_SOUND_IDLE,FLOURISH_SOUND_IDLE
};


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


extern void AddSong(unsigned int song_id);

bool HardStop = false, SoftStop = false, SofterStop = false;

HANDLE hSWGPlayerMutex;

typedef struct {
  void           * Next;
  SWGPlayerCmdType Command;
  DWORD            Payload[2];
} SWGPlayerCmdQueueType;

SWGPlayerCmdQueueType * CmdQueue = NULL;

bool SWGPlayer(HWND hWnd, HANDLE * SWGPlayerH)
{
  if (!registry->find("."))
  {
    return false;
  }

  for (std::set<int>::iterator it = registry->songs.begin(); it != registry->songs.end(); ++it)
  {
    AddSong(*it);
  }


  // Create a mutex with no initial owner.

  hSWGPlayerMutex = CreateMutex(
    NULL,                       // no security attributes
    FALSE,                      // initially not owned
    "SWGPlayer");  // name of mutex

  if (hSWGPlayerMutex == NULL)
  {
    TRACE("ERROR: No Mutex.");
    return false;
  }

  ThreadHandle = CreateThread(
    NULL,
    0,
    SWGPlayerProc,
    (LPVOID)hWnd,
    0,
    &player_hWnd
  );

  if (SWGPlayerH != NULL)
  {
    *SWGPlayerH = ThreadHandle;
  }
  return true;
}




bool PlayerCmdQPop(SWGPlayerCmdType * Command, DWORD * Payload1, DWORD * Payload2)
{
  DWORD WaitType = 0;
  SWGPlayerCmdQueueType * p = NULL;

  WaitType = WaitForSingleObject(hSWGPlayerMutex, 100L);

  if (WaitType == WAIT_OBJECT_0)
  {
    p = CmdQueue;
    if (p != NULL)
    {
      *Command = p->Command;
      *Payload1 = p->Payload[0];
      *Payload2 = p->Payload[1];

      CmdQueue = reinterpret_cast<SWGPlayerCmdQueueType*>(p->Next);

      HeapFree(GetProcessHeap(), 0, p);

      ReleaseMutex(hSWGPlayerMutex);
      return true;
    }
    ReleaseMutex(hSWGPlayerMutex);
  }

  return false;
}




bool PlayerCmdQPush(SWGPlayerCmdType Command, DWORD Payload1, DWORD Payload2)
{
  SWGPlayerCmdQueueType * p = NULL;
  DWORD WaitType = 0;

  WaitType = WaitForSingleObject(hSWGPlayerMutex, 100L);

  if (WaitType == WAIT_OBJECT_0)
  {
    SWGPlayerCmdQueueType * CmdEntry = reinterpret_cast<SWGPlayerCmdQueueType *>(HeapAlloc(GetProcessHeap(), 0, sizeof(SWGPlayerCmdQueueType)));
    CmdEntry->Command = Command;
    CmdEntry->Payload[0] = Payload1;
    CmdEntry->Payload[1] = Payload2;
    CmdEntry->Next = NULL;

    p = reinterpret_cast<SWGPlayerCmdQueueType*>(CmdQueue);

    if (p != NULL)
    {
      while (p->Next != NULL)
      {
        p = reinterpret_cast<SWGPlayerCmdQueueType*>(p->Next);
      }
      p->Next = reinterpret_cast<void*>(CmdEntry);
    }
    else
    {
      CmdQueue = CmdEntry;
    }

    ReleaseMutex(hSWGPlayerMutex);
    return true;
  }

  return false;
}

bool EnableRandomFlourish(bool enable)
{
  if (true == PlayerCmdQPush(
    SWG_PLAYER_CMD_SET_RANDOM_FLOURISH,
    static_cast<DWORD>(enable),
    0
  )
    )
  {
    return true;
  }
  return false;
}

bool SWGSetDialog(HWND hDlg)
{
  if (true == PlayerCmdQPush(
    SWG_PLAYER_CMD_SET_DIALOG_HANDLE,
    reinterpret_cast<DWORD>(hDlg),
    0
  )
    )
  {
    return true;
  }
  return false;
}


bool Flourish(InstrumentSoundType Instrument, FlourishSoundType Flourish)
{
  if (Flourish < FLOURISH_SOUND_MAX)
  {
    if (true == PlayerCmdQPush(
      SWG_PLAYER_CMD_FLOURISH,
      static_cast<DWORD>(Instrument),
      static_cast<DWORD>(Flourish)
    )
      )
    {
      return true;
    }
  }
  return false;
}



bool BandFlourish(FlourishSoundType Flourish)
{
  if (Flourish < FLOURISH_SOUND_MAX)
  {
    if (true == PlayerCmdQPush(
      SWG_PLAYER_CMD_BAND_FLOURISH,
      static_cast<DWORD>(Flourish),
      0
    )
      )
    {
      return true;
    }
  }
  return false;
}



bool ChangeMusic(SongSoundType SongNum)
{
  if (SongNum < SONG_SOUND_MAX)
  {
    if (true == PlayerCmdQPush(
      SWG_PLAYER_CMD_CHANGE,
      static_cast<DWORD>(SongNum),
      0
    )
      )
    {
      return true;
    }
  }
  TRACE("SWGPlayer: ChangeMusic: Bad song: %d/%d", SongNum, SONG_SOUND_MAX);
  return false;
}

bool Play(SongSoundType SongNum, FlourishSoundType FlourishNum)
{
  if (SongNum < SONG_SOUND_MAX && FlourishNum < FLOURISH_SOUND_MAX)
  {
    if (true == PlayerCmdQPush(
      SWG_PLAYER_CMD_PLAY,
      static_cast<DWORD>(SongNum),
      static_cast<DWORD>(FlourishNum)
    )
      )
    {
      return true;
    }
  }
  TRACE("SWGPlayer: Play: Bad song: %d/%d", SongNum, SONG_SOUND_MAX);
  return false;
}



bool EnableInstrument(InstrumentSoundType Inst, bool enable, SWGPlayerStopType stop)
{
  if (Inst < INSTRUMENT_SOUND_MAX)
  {
    if (enable == true)
    {
      if (true == PlayerCmdQPush(
        SWG_PLAYER_CMD_ENABLE_INSTRUMENT,
        static_cast<DWORD>(Inst),
        static_cast<DWORD>(SWG_PLAYER_STOP_MAX)
      )
        )
      {
        return true;
      }
    }
    else
    {
      if (true == PlayerCmdQPush(
        SWG_PLAYER_CMD_ENABLE_INSTRUMENT,
        static_cast<DWORD>(Inst),
        static_cast<DWORD>(stop)
      )
        )
      {
        return true;
      }
    }
  }
  TRACE("SWGPlayer: Stop: Bad instrument: %d/%d", Inst, INSTRUMENT_SOUND_MAX);
  return false;
}


bool Stop(SWGPlayerStopType StopType)
{
  if (StopType < SWG_PLAYER_STOP_MAX)
  {
    if (true == PlayerCmdQPush(
      SWG_PLAYER_CMD_STOP,
      static_cast<DWORD>(StopType),
      0
    )
      )
    {
      return true;
    }
  }
  TRACE("SWGPlayer: Stop: Bad stop type: %d/%d", StopType, SWG_PLAYER_STOP_MAX);
  return false;
}

void SendPlaylistUpdate(FlourishBufferType * FloBuff, SoundListType * NowPlaying)
{
  static SoundListType Buffer[2];

  for (int x = 0; x < 2; x++)
  {
    for (int i = 0; i < NUM_INSTRUMENTS; i++)
    {
      Buffer[x].SoundList[i] = FLOURISH_SOUND_MAX;
    }
  }

  bool AllowRandom = true;

  for (int x = 0; x < NUM_INSTRUMENTS; x++)
  {
    if (EnabledInstrument[x] == true)
    {
      Buffer[0].SoundList[x] = NowPlaying->SoundList[x];
    }
    else
    {
      Buffer[0].SoundList[x] = FLOURISH_SOUND_SILENCE;
    }
  }


  for (int x = 0; x < NUM_INSTRUMENTS; x++)
  {
    if (EnabledInstrument[x] == true)
    {
      if (FloBuff->BandFlo == true)
      {
        AllowRandom = false;
        Buffer[1].SoundList[x] = FloBuff->BandBuffer.SoundList[x];
      }
      else
      {
        if (FloBuff->UsedFloBuffs[x] == 0)
        {
          Buffer[1].SoundList[x] = FLOURISH_SOUND_IDLE;
        }
        else
        {
          if (FloBuff->Buffer[FloBuff->CurrFloIndex[x]].SoundList[x] == FLOURISH_SOUND_STOP)
          {
            //EnabledInstrument[x] = false;
            Buffer[1].SoundList[x] = FLOURISH_SOUND_SILENCE;
            //FloBuff->UsedFloBuffs[x] = 0;
          }
          else
          {
            AllowRandom = false;
            Buffer[1].SoundList[x] = FloBuff->Buffer[FloBuff->CurrFloIndex[x]].SoundList[x];
          }
          /*
          FloBuff->CurrFloIndex[x]++;
          if (FloBuff->CurrFloIndex[x] >= MAX_FLOURISH_BUFFERS)
          {
            FloBuff->CurrFloIndex[x] = 0;
          }
          FloBuff->UsedFloBuffs[x]--;
          */
        }
      }
    }
  }

  //FloBuff->BandFlo = false;


  if (RandomFlourish && AllowRandom)
  {
    for (int x = 0; x < NUM_INSTRUMENTS; x++)
    {
      if (EnabledInstrument[x] == true)
      {
        Buffer[1].SoundList[x] = FloBuff->RandBuffer.SoundList[x];
      }
      else
      {
        Buffer[1].SoundList[x] = FLOURISH_SOUND_SILENCE;
      }
    }
  }


  if (client_hDlg != 0)
  {
    LRESULT res = SendMessage(client_hDlg, SWGC_STATUS, (WPARAM)0, (LPARAM)Buffer);
  }
}



Song *song = NULL;

DWORD WINAPI SWGPlayerProc(LPVOID lpParameter)
{
  FlourishBufferType FlourishBuffer;
  bool InPlayback = false;
  bool bInstruments[NUM_INSTRUMENTS] =
  { true, true, true, true, true, true };
  bool bPendingSong = false;
  SoundListType CurrFlo;

  TRACE("SWGPlayer: THREAD STARTED!\n");

  srand((unsigned int)time(NULL));
  client_hWnd = (HWND)lpParameter;
  CreateDSoundObject();

  InitFlourish(&FlourishBuffer);

  for (;;)
  {
    SWGPlayerCmdType Command;
    DWORD            Payload[2];

    if (!InPlayback)
    {
      Sleep(100L);
    }

    //FlourishBufferType * stuff = reinterpret_cast<FlourishBufferType *>(HeapAlloc(GetProcessHeap(),0,sizeof(FlourishBufferType)));
    //memcpy(stuff,&FlourishBuffer,sizeof(FlourishBufferType));

    if (true == PlayerCmdQPop(&Command, &Payload[0], &Payload[1]))
    {
      switch (Command)
      {
      case SWG_PLAYER_CMD_PLAY:
        if (!InPlayback)
        {
          bPendingSong = false;
          InPlayback = true;
          SongNum = static_cast<SongSoundType>(Payload[0]);
          FlourishSoundType StartFlourish = static_cast<FlourishSoundType>(Payload[1]);
          //LoadSongFiles(SongNum);
          if (song)
          {
            delete song;
            song = NULL;
          }
          song = new Song(SongNum, registry);
          song->prepare();

          InitFlourish(&FlourishBuffer);
          int numInst = 0;

          for (int x = 0; x < NUM_INSTRUMENTS; x++)
          {
            if (bInstruments[x] == true)
            {
              numInst += InsertFlourish(&FlourishBuffer, (InstrumentSoundType)x, FLOURISH_SOUND_START);
            }
          }

          if (numInst == 0)
          {
            InPlayback = false;
          }
          else
          {
            for (int x = 0; x < NUM_INSTRUMENTS; x++)
            {
              if (bInstruments[x] == true)
              {
                numInst += InsertFlourish(&FlourishBuffer, (InstrumentSoundType)x, StartFlourish);
              }
            }
          }
          SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
        }
        else if (!bPendingSong)
        {
          bPendingSong = true;
          for (int x = 0; x < INSTRUMENT_SOUND_MAX; x++)
          {
            SoftStop = true;
            if (DSBuffer[x] != NULL)
            {
              DSBuffer[x]->Stop();
            }
          }
          Play(static_cast<SongSoundType>(Payload[0]), FLOURISH_SOUND_IDLE);
        }
        break;
      case SWG_PLAYER_CMD_STOP:
        if (InPlayback)
        {
          SWGPlayerStopType StopType = static_cast<SWGPlayerStopType>(Payload[0]);
          switch (StopType)
          {
          case SWG_PLAYER_STOP_GRACEFUL:
            SofterStop = true;
            InsertBandFlourish(&FlourishBuffer, FLOURISH_SOUND_OUTRO, true);
            SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
            break;

          case SWG_PLAYER_STOP_END_OF_PHRASE:
            SoftStop = true;
            break;

          case SWG_PLAYER_STOP_NOW:
            for (int x = 0; x < INSTRUMENT_SOUND_MAX; x++)
            {
              SoftStop = true;
              if (DSBuffer[x] != NULL)
              {
                DSBuffer[x]->Stop();
              }
            }
            break;
          default:
            break;
          }
        }
        break;
      case SWG_PLAYER_CMD_FLOURISH:
        if (InPlayback)
        {
          InstrumentSoundType Inst = static_cast<InstrumentSoundType>(Payload[0]);
          FlourishSoundType Flo = static_cast<FlourishSoundType>(Payload[1]);
          InsertFlourish(&FlourishBuffer, Inst, Flo);
          SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
        }
        break;
      case SWG_PLAYER_CMD_BAND_FLOURISH:
        if (InPlayback)
        {
          FlourishSoundType Flo = static_cast<FlourishSoundType>(Payload[0]);
          InsertBandFlourish(&FlourishBuffer, Flo, false);
          SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
        }
        break;
      case SWG_PLAYER_CMD_CHANGE:
        if (InPlayback && !bPendingSong)
        {
          bPendingSong = true;
          SoftStop = true;
          Play(static_cast<SongSoundType>(Payload[0]), FLOURISH_SOUND_IDLE);
        }
        break;
      case SWG_PLAYER_CMD_ENABLE_INSTRUMENT:
      {
        int numInst = 0;
        InstrumentSoundType Inst = static_cast<InstrumentSoundType>(Payload[0]);
        SWGPlayerStopType stop = static_cast<SWGPlayerStopType>(Payload[1]);

        if (stop == SWG_PLAYER_STOP_MAX && bInstruments[Inst] != true)
        {
          bInstruments[Inst] = true;
          InsertFlourish(&FlourishBuffer, Inst, FLOURISH_SOUND_START, true);
          SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
        }
        else if (bInstruments[Inst] != false && stop != SWG_PLAYER_STOP_MAX)
        {
          bInstruments[Inst] = false;

          switch (stop)
          {
          case SWG_PLAYER_STOP_GRACEFUL:
            if (InPlayback)
            {
              InsertFlourish(&FlourishBuffer, Inst, FLOURISH_SOUND_OUTRO, true);
            }
            InsertFlourish(&FlourishBuffer, Inst, FLOURISH_SOUND_STOP);
            SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
            break;
          case SWG_PLAYER_STOP_NOW:
            if (InPlayback)
            {
              if (DSBuffer[Inst] != NULL)
              {
                DSBuffer[Inst]->Stop();
              }
            }
            break;
          case SWG_PLAYER_STOP_END_OF_PHRASE:
            InsertFlourish(&FlourishBuffer, Inst, FLOURISH_SOUND_STOP, true);
            SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
            break;
          default:
            break;
          }
        }

        for (int i = 0; i < NUM_INSTRUMENTS; i++)
        {
          if (bInstruments[i] == true)
          {
            numInst++;
          }
        }
        if (numInst == 0 && InPlayback)
        {
          // Stop NOW
          for (int x = 0; x < INSTRUMENT_SOUND_MAX; x++)
          {
            SoftStop = true;
            if (DSBuffer[x] != NULL)
            {
              DSBuffer[x]->Stop();
            }
          }

          for (int x = 0; x < NUM_INSTRUMENTS; x++)
          {
            EnabledInstrument[x] = false;
          }

        }
      }
      break;
      case SWG_PLAYER_CMD_SET_RANDOM_FLOURISH:
        RandomFlourish = (Payload[0] != 0);
        break;
      case SWG_PLAYER_CMD_SET_DIALOG_HANDLE:
        client_hDlg = reinterpret_cast<HWND>(Payload[0]);
      default:
        break;
      }
    }

    if (InPlayback)
    {
      do
      {
        Sleep(100L);

        if (CheckPhrasePlaybackStatus() == true)
        {
          continue;
        }

        if (SoftStop == true)
        {
          ReleaseSoundBuffers();
          SoftStop = false; InPlayback = false;
          for (int x = 0; x < NUM_INSTRUMENTS; x++)
          {
            EnabledInstrument[x] = false;
          }
          continue;
        }

        //SoundListType CurrFlo;
        int DoneCount = GetNextFlourish(&CurrFlo, &FlourishBuffer);

        if (DoneCount == NUM_INSTRUMENTS)
        {
          if (SofterStop == true)
          {
            ReleaseSoundBuffers();
            SofterStop = false; InPlayback = false;
            continue;
          }
        }

        PlayNextFlourish(CurrFlo);
        SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
      } while (bPendingSong && InPlayback);
    }
  }


  ExitThread(0);
}

void GenerateRandomSet(FlourishBufferType * FloBuff)
{
  int randnum = rand();
  int allowmixed = (int)(((float)randnum / (float)RAND_MAX)*4.0);
  if (!allowmixed)
  {
    for (int x = 0; x < NUM_INSTRUMENTS; x++)
    {
      randnum = rand();
      int nonsilent = (int)(((float)randnum / (float)RAND_MAX)*8.0);
      if (nonsilent)
      {
        int floval = (int)(((float)rand() / (float)RAND_MAX)*16.0);
        if (static_cast<FlourishSoundType>(floval) == FLOURISH_SOUND_INTRO
          || static_cast<FlourishSoundType>(floval) == FLOURISH_SOUND_OUTRO
          || static_cast<FlourishSoundType>(floval) >= FLOURISH_SOUND_MAX)
        {
          floval = FLOURISH_SOUND_IDLE;
        }
        FloBuff->RandBuffer.SoundList[x] = static_cast<FlourishSoundType>(floval);
      }
      else
      {
        FloBuff->RandBuffer.SoundList[x] = FLOURISH_SOUND_SILENCE;
      }
    }
  }
  else
  {
    FlourishSoundType floval = static_cast<FlourishSoundType>((int)(((float)rand() / (float)RAND_MAX)*(float)FLOURISH_SOUND_MAX));
    if (floval == FLOURISH_SOUND_INTRO
      || floval == FLOURISH_SOUND_OUTRO)
    {
      floval = FLOURISH_SOUND_IDLE;
    }

    for (int x = 0; x < NUM_INSTRUMENTS; x++)
    {
      FloBuff->RandBuffer.SoundList[x] = floval;
    }
  }
}


void InitFlourish(FlourishBufferType * FloBuff)
{
  for (int x = 0; x < NUM_INSTRUMENTS; x++)
  {
    FloBuff->UsedFloBuffs[x] = 0;
    FloBuff->CurrFloIndex[x] = 0;
    FloBuff->CurrInsertLoc[x] = 0;
    FloBuff->BandBuffer.SoundList[x] = FLOURISH_SOUND_IDLE;
    for (int i = 0; i < MAX_FLOURISH_BUFFERS; i++)
    {
      FloBuff->Buffer[i].SoundList[x] = FLOURISH_SOUND_SILENCE;
    }
  }
  FloBuff->BandFlo = false;
  GenerateRandomSet(FloBuff);
}

int InsertFlourish(FlourishBufferType * FloBuff, InstrumentSoundType Instrument, FlourishSoundType Flourish, bool FlushQueue)
{
  if (FlushQueue)
  {
    FloBuff->UsedFloBuffs[Instrument] = 0;
  }

  if (FloBuff->UsedFloBuffs[Instrument] < MAX_FLOURISH_BUFFERS)
  {
    FloBuff->Buffer[FloBuff->CurrInsertLoc[Instrument]].SoundList[Instrument] = Flourish;
    FloBuff->CurrInsertLoc[Instrument]++;
    if (FloBuff->CurrInsertLoc[Instrument] >= MAX_FLOURISH_BUFFERS)
    {
      FloBuff->CurrInsertLoc[Instrument] = 0;
    }

    FloBuff->UsedFloBuffs[Instrument]++;
  }
  else
  {
    return 0;
  }

  return 1;
}


int InsertBandFlourish(FlourishBufferType * FloBuff, FlourishSoundType Flourish, bool Final)
{
  int DoneCount = 0;

  for (int x = 0; x < NUM_INSTRUMENTS; x++)
  {
    FloBuff->BandBuffer.SoundList[x] = Flourish;

    if (Final == true)
    {
      InsertFlourish(FloBuff, (InstrumentSoundType)x, FLOURISH_SOUND_STOP, true);
    }
  }

  FloBuff->BandFlo = true;

  return DoneCount;
}

int GetNextFlourish(SoundListType * DestBuff, FlourishBufferType * FloBuff)
{

  int DoneCount = 0;
  bool AllowRandom = true;

  for (int x = 0; x < NUM_INSTRUMENTS; x++)
  {
    if (EnabledInstrument[x] == true)
    {
      if (FloBuff->BandFlo == true)
      {
        AllowRandom = false;
        DestBuff->SoundList[x] = FloBuff->BandBuffer.SoundList[x];
      }
      else
      {
        if (FloBuff->UsedFloBuffs[x] == 0)
        {
          DoneCount++;
          DestBuff->SoundList[x] = FLOURISH_SOUND_IDLE;
        }
        else
        {
          if (FloBuff->Buffer[FloBuff->CurrFloIndex[x]].SoundList[x] == FLOURISH_SOUND_STOP)
          {
            EnabledInstrument[x] = false;
            DestBuff->SoundList[x] = FLOURISH_SOUND_SILENCE;
            FloBuff->UsedFloBuffs[x] = 0;
          }
          else
          {
            AllowRandom = false;
            DestBuff->SoundList[x] = FloBuff->Buffer[FloBuff->CurrFloIndex[x]].SoundList[x];
          }
          FloBuff->CurrFloIndex[x]++;
          if (FloBuff->CurrFloIndex[x] >= MAX_FLOURISH_BUFFERS)
          {
            FloBuff->CurrFloIndex[x] = 0;
          }
          FloBuff->UsedFloBuffs[x]--;
        }
      }
    }
    else
    {
      if (FloBuff->Buffer[FloBuff->CurrFloIndex[x]].SoundList[x] == FLOURISH_SOUND_START && FloBuff->UsedFloBuffs[x] > 0)
      {
        AllowRandom = false;
        EnabledInstrument[x] = true;

        FloBuff->CurrFloIndex[x]++;
        if (FloBuff->CurrFloIndex[x] >= MAX_FLOURISH_BUFFERS)
        {
          FloBuff->CurrFloIndex[x] = 0;
        }
        FloBuff->UsedFloBuffs[x]--;

        x--;
      }
      else
      {
        DoneCount++;
        DestBuff->SoundList[x] = FLOURISH_SOUND_SILENCE;
        FloBuff->UsedFloBuffs[x] = 0;
      }
    }
  }

  FloBuff->BandFlo = false;


  if (RandomFlourish && AllowRandom)
  {
    for (int x = 0; x < NUM_INSTRUMENTS; x++)
    {
      if (EnabledInstrument[x] == true)
      {
        DestBuff->SoundList[x] = FloBuff->RandBuffer.SoundList[x];
      }
    }
    GenerateRandomSet(FloBuff);
  }

  return DoneCount;
}

bool ReleaseSoundBuffers()
{
  int            NumInstruments = INSTRUMENT_SOUND_MAX;
  /*********************************
    Release Sound Buffers
  *********************************/
  for (int i = 0; i < NumInstruments; i++)
  {
    if (CurrentFlourish.SoundList[i] != -1)
    {
      if (DSBuffer[i] != NULL)
      {
        DSBuffer[i]->Release();
        DSBuffer[i] = NULL;
      }
    }
  }

  //for (int i = 0; i < (int)NumInstruments; i++)
  //{
  //  for (int f = 0; f < (int)FLOURISH_SOUND_MAX; f++)
  //  {
  //    HeapFree(GetProcessHeap(), 0, FileBuffer[i][f]);
  //  }
  //}
  return true;
}

bool CreateDSoundObject()
{
  HRESULT        r;

  /*********************************
    Create DSound Interface Object
   *********************************/
  TRACE("DSOUND: Beginning Dsound...\n");

  r = DirectSoundCreate8(
    NULL, // Use default device
    &DSound,
    NULL
  );

  if (DS_OK != r)
  {
    TRACE("DSOUND: ERROR: Couldn't load DSound interface: %d\n", r);
    return false;
  }

  r = DSound->SetCooperativeLevel(client_hWnd, DSSCL_NORMAL); //DSSCL_PRIORITY);
  if (DS_OK != r)
  {
    TRACE("DSOUND: ERROR: Couldn't load DSound interface: %d\n", r);
    return false;
  }

  return true;
}

bool CheckPhrasePlaybackStatus()
{
  DWORD TempBuffStatus = 0;
  int   NumInstruments = INSTRUMENT_SOUND_MAX;
  int i;

  if (HardStop == true)
    return false;

  // Update BuffStatus
  for (i = 0; i < NumInstruments; i++)
  {
    if (CurrentFlourish.SoundList[i] != -1 && ((1 << i) & BuffStatus) == 0)
    {
      if (DSBuffer[i] != NULL)
      {
        DSBuffer[i]->GetStatus(&TempBuffStatus);
        BuffStatus |= (~TempBuffStatus & DSBSTATUS_PLAYING) << i;
        if ((~TempBuffStatus & DSBSTATUS_PLAYING) > 0)
        {
          DSBuffer[i]->Release();
          DSBuffer[i] = NULL;
        }
      }
    }
  }

  if (BuffStatus != InstrumentBitMask)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void PlayNextFlourish(SoundListType PhraseList)
{
  HRESULT        r;
  int   NumInstruments = INSTRUMENT_SOUND_MAX;
  int            i;

  for (i = 0; i < NUM_INSTRUMENTS; i++)
  {
    CurrentFlourish.SoundList[i] = PhraseList.SoundList[i];
  }

  /*********************************
            Play Buffer
   *********************************/
  BuffStatus = 0;
  InstrumentBitMask = 0;

  /*********************************
    Create Sound Buffer
  *********************************/
  for (i = 0; i < NumInstruments; i++)
  {
    if (CurrentFlourish.SoundList[i] != FLOURISH_SOUND_SILENCE
      && song->dsBufferDesc((InstrumentSoundType)i, CurrentFlourish.SoundList[i])->dwBufferBytes > 0)
    {
      r = DSound->CreateSoundBuffer(
        song->dsBufferDesc((InstrumentSoundType)i, CurrentFlourish.SoundList[i]),
        &DSBuffer[i],
        NULL
      );

      if (DS_OK != r)
      {
        TRACE("DSOUND: ERROR: Couldn't create DSound[%d] buffer: %d\n", i, r);
        return;
      }
    }
  }

  /*********************************
      Transfer Buffer
  *********************************/
  for (i = 0; i < NumInstruments; i++)
  {
    if (CurrentFlourish.SoundList[i] != FLOURISH_SOUND_SILENCE
      && song->dsBufferDesc((InstrumentSoundType)i, CurrentFlourish.SoundList[i])->dwBufferBytes > 0)
    {
      void * FirstPage = NULL;
      DWORD  FirstPageSize = 0;
      void * SecondPage = NULL;
      DWORD  SecondPageSize = 0;
      DWORD  RemainingData = 0;

      r = DSBuffer[i]->Lock(
        0,
        song->dsBufferDesc((InstrumentSoundType)i, CurrentFlourish.SoundList[i])->dwBufferBytes,
        &FirstPage,
        &FirstPageSize,
        &SecondPage,
        &SecondPageSize,
        DSBLOCK_ENTIREBUFFER
      );

      if (DS_OK != r)
      {
        TRACE("DSOUND: ERROR: Couldn't lock DSound buffer: %d\n", r);
        return;
      }

      memcpy(FirstPage, song->data((InstrumentSoundType)i, CurrentFlourish.SoundList[i]), song->dsBufferDesc((InstrumentSoundType)i, CurrentFlourish.SoundList[i])->dwBufferBytes);
      if (FirstPageSize <= song->dsBufferDesc((InstrumentSoundType)i, CurrentFlourish.SoundList[i])->dwBufferBytes)
      {
        RemainingData = song->dsBufferDesc((InstrumentSoundType)i, CurrentFlourish.SoundList[i])->dwBufferBytes - FirstPageSize;
      }
      else
      {
        RemainingData = 0;
      }

      if (SecondPageSize > 0
        && RemainingData > 0
        && RemainingData < SecondPageSize
        )
      {
        memcpy(
          SecondPage,
          ((BYTE *)song->data((InstrumentSoundType)i, CurrentFlourish.SoundList[i])) + FirstPageSize,
          RemainingData
        );
      }

      r = DSBuffer[i]->Unlock(
        FirstPage,
        FirstPageSize,
        SecondPage,
        RemainingData
      );

      if (DS_OK != r)
      {
        TRACE("DSOUND: ERROR: Couldn't unlock DSound buffer: %d\n", r);
        return;
      }
    }
  }


  for (i = 0; i < NumInstruments; i++)
  {
    if (CurrentFlourish.SoundList[i] != FLOURISH_SOUND_SILENCE
      && song->dsBufferDesc((InstrumentSoundType)i, CurrentFlourish.SoundList[i])->dwBufferBytes > 0)
    {
      r = DSBuffer[i]->Play(0, 0, 0);

      if (DS_OK != r)
      {
        TRACE("DSOUND: ERROR: Couldn't play DSound buffer: %d\n", r);
        return;
      }
    }
  }

  InstrumentBitMask = 0;
  for (i = 0; i < NumInstruments; i++)
  {
    if (CurrentFlourish.SoundList[i] != FLOURISH_SOUND_SILENCE && EnabledInstrument[i] == true
      && song->dsBufferDesc((InstrumentSoundType)i, CurrentFlourish.SoundList[i])->dwBufferBytes > 0)
    {
      InstrumentBitMask |= 1 << i;
    }
  }

}


bool SWGSetDialog(HWND hDlg);














#ifdef FEATURE_PLAYER_CLASS
SWGPlayer::SWGPlayer(HWND hWnd)
{
  client_hWnd = hWnd;

  if (registry->find("."))
  {
    hSWGPlayerMutex = CreateMutex(
      NULL,
      FALSE,
      "SWGPlayer");

    if (hSWGPlayerMutex != NULL)
    {
      ThreadHandle = CreateThread(
        NULL,
        0,
        SWGPlayerProc,
        (LPVOID)hWnd,
        0,
        &player_hWnd
      );
    }
  }
}
#endif /* FEATURE_PLAYER_CLASS */



