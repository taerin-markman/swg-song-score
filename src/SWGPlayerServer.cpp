#include "stdafx.h"
#include <dsound.h>
#include <stdio.h>
#include <time.h>
#include "SWGPlayerServer.h"
#include "Debug.h"

SWGPlayerServer::SWGPlayerServer(HWND hWnd, ResourceRegistry *registry)
{
  this->Registry = registry;
  this->ClientHWND = hWnd;

  this->hMutex = CreateMutex(
    NULL,
    FALSE,
    "SWGPlayer");

  if (this->hMutex != NULL)
  {
    this->ThreadHandle = CreateThread(
      NULL,
      0,
      SWGPlayerServer::SWGPlayerProc,
      (LPVOID)this,
      0,
      NULL
    );
  }
}

bool SWGPlayerServer::SendCommand(SWGPlayerCmdType Command, DWORD Payload1, DWORD Payload2)
{
  SWGPlayerCmdQueueType * p = NULL;
  DWORD WaitType = 0;

  WaitType = WaitForSingleObject(this->hMutex, 100L);

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

    ReleaseMutex(this->hMutex);
    return true;
  }

  return false;
}

bool SWGPlayerServer::GetCommand(SWGPlayerCmdType * Command, DWORD * Payload1, DWORD * Payload2)
{
  DWORD WaitType = 0;
  SWGPlayerCmdQueueType * p = NULL;

  WaitType = WaitForSingleObject(this->hMutex, 100L);

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

      ReleaseMutex(this->hMutex);
      return true;
    }
    ReleaseMutex(this->hMutex);
  }

  return false;
}



void SWGPlayerServer::SendPlaylistUpdate(FlourishBufferType * FloBuff, SoundListType * NowPlaying)
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
    if (this->EnabledInstrument[x] == true)
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
    if (this->EnabledInstrument[x] == true)
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
            Buffer[1].SoundList[x] = FLOURISH_SOUND_SILENCE;
          }
          else
          {
            AllowRandom = false;
            Buffer[1].SoundList[x] = FloBuff->Buffer[FloBuff->CurrFloIndex[x]].SoundList[x];
          }
        }
      }
    }
  }

  if (this->RandomizerEnabled && AllowRandom)
  {
    for (int x = 0; x < NUM_INSTRUMENTS; x++)
    {
      if (this->EnabledInstrument[x] == true)
      {
        Buffer[1].SoundList[x] = FloBuff->RandBuffer.SoundList[x];
      }
      else
      {
        Buffer[1].SoundList[x] = FLOURISH_SOUND_SILENCE;
      }
    }
  }


  if (this->StatusHWND != 0)
  {
    LRESULT res = SendMessage(this->StatusHWND, SWGC_STATUS, (WPARAM)0, (LPARAM)Buffer);
  }
}

DWORD WINAPI SWGPlayerServer::SWGPlayerProc(LPVOID lpParameter)
{
  SWGPlayerServer *server = reinterpret_cast<SWGPlayerServer *>(lpParameter);
  FlourishBufferType FlourishBuffer;
  bool InPlayback = false;
  bool bInstruments[NUM_INSTRUMENTS] =
  { true, true, true, true, true, true };
  bool bPendingSong = false;
  SoundListType CurrFlo;

  server->Registry->find(".");

  TRACE("SWGPlayer: THREAD STARTED!\n");

  srand((unsigned int)time(NULL));
  server->CreateDSoundObject();

  server->InitFlourish(&FlourishBuffer);

  for (;;)
  {
    SWGPlayerCmdType Command;
    DWORD            Payload[2];

    if (!InPlayback)
    {
      Sleep(100L);
    }

    if (TRUE == server->GetCommand(&Command, &Payload[0], &Payload[1]))
    {
      switch (Command)
      {
      case SWG_PLAYER_CMD_PLAY:
        if (!InPlayback)
        {
          bPendingSong = false;
          InPlayback = true;
          FlourishSoundType StartFlourish = static_cast<FlourishSoundType>(Payload[1]);
          if (server->CurrentSong)
          {
            delete server->CurrentSong;
            server->CurrentSong = NULL;
          }
          server->CurrentSong = new Song(static_cast<SongSoundType>(Payload[0]), server->Registry);
          server->CurrentSong->prepare();

          server->InitFlourish(&FlourishBuffer);
          int numInst = 0;

          for (int x = 0; x < NUM_INSTRUMENTS; x++)
          {
            if (bInstruments[x] == true)
            {
              numInst += server->InsertFlourish(&FlourishBuffer, (InstrumentSoundType)x, FLOURISH_SOUND_START);
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
                numInst += server->InsertFlourish(&FlourishBuffer, (InstrumentSoundType)x, StartFlourish);
              }
            }
          }
          server->SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
        }
        else if (!bPendingSong)
        {
          bPendingSong = true;
          for (int x = 0; x < INSTRUMENT_SOUND_MAX; x++)
          {
            server->SoftStop = true;
            if (server->DSBuffer[x] != NULL)
            {
              server->DSBuffer[x]->Stop();
            }
          }
          SongSoundType newSong = static_cast<SongSoundType>(Payload[0]);
          if (newSong < SONG_SOUND_MAX)
          {
            server->SendCommand(
              SWG_PLAYER_CMD_PLAY,
              static_cast<DWORD>(newSong),
              static_cast<DWORD>(FLOURISH_SOUND_IDLE)
            );
          }
        }
        break;
      case SWG_PLAYER_CMD_STOP:
        if (InPlayback)
        {
          SWGPlayerStopType StopType = static_cast<SWGPlayerStopType>(Payload[0]);
          switch (StopType)
          {
          case SWG_PLAYER_STOP_GRACEFUL:
            server->SofterStop = true;
            server->InsertBandFlourish(&FlourishBuffer, FLOURISH_SOUND_OUTRO, true);
            server->SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
            break;

          case SWG_PLAYER_STOP_END_OF_PHRASE:
            server->SoftStop = true;
            break;

          case SWG_PLAYER_STOP_NOW:
            for (int x = 0; x < INSTRUMENT_SOUND_MAX; x++)
            {
              server->SoftStop = true;
              if (server->DSBuffer[x] != NULL)
              {
                server->DSBuffer[x]->Stop();
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
          server->InsertFlourish(&FlourishBuffer, Inst, Flo);
          server->SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
        }
        break;
      case SWG_PLAYER_CMD_BAND_FLOURISH:
        if (InPlayback)
        {
          FlourishSoundType Flo = static_cast<FlourishSoundType>(Payload[0]);
          server->InsertBandFlourish(&FlourishBuffer, Flo, false);
          server->SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
        }
        break;
      case SWG_PLAYER_CMD_CHANGE:
        if (InPlayback && !bPendingSong)
        {
          bPendingSong = true;
          server->SoftStop = true;
          SongSoundType newSong = static_cast<SongSoundType>(Payload[0]);
          if (newSong < SONG_SOUND_MAX)
          {
            server->SendCommand(
              SWG_PLAYER_CMD_PLAY,
              static_cast<DWORD>(newSong),
              static_cast<DWORD>(FLOURISH_SOUND_IDLE)
            );
          }
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
          server->InsertFlourish(&FlourishBuffer, Inst, FLOURISH_SOUND_START, true);
          server->SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
        }
        else if (bInstruments[Inst] != false && stop != SWG_PLAYER_STOP_MAX)
        {
          bInstruments[Inst] = false;

          switch (stop)
          {
          case SWG_PLAYER_STOP_GRACEFUL:
            if (InPlayback)
            {
              server->InsertFlourish(&FlourishBuffer, Inst, FLOURISH_SOUND_OUTRO, true);
            }
            server->InsertFlourish(&FlourishBuffer, Inst, FLOURISH_SOUND_STOP);
            server->SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
            break;
          case SWG_PLAYER_STOP_NOW:
            if (InPlayback)
            {
              if (server->DSBuffer[Inst] != NULL)
              {
                server->DSBuffer[Inst]->Stop();
              }
            }
            break;
          case SWG_PLAYER_STOP_END_OF_PHRASE:
            server->InsertFlourish(&FlourishBuffer, Inst, FLOURISH_SOUND_STOP, true);
            server->SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
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
            server->SoftStop = true;
            if (server->DSBuffer[x] != NULL)
            {
              server->DSBuffer[x]->Stop();
            }
          }

          for (int x = 0; x < NUM_INSTRUMENTS; x++)
          {
            server->EnabledInstrument[x] = false;
          }

        }
      }
      break;
      case SWG_PLAYER_CMD_SET_RANDOM_FLOURISH:
        server->RandomizerEnabled = (Payload[0] != 0);
        break;
      case SWG_PLAYER_CMD_SET_DIALOG_HANDLE:
        server->StatusHWND = reinterpret_cast<HWND>(Payload[0]);
        for (std::set<int>::iterator it = server->Registry->songs.begin(); it != server->Registry->songs.end(); ++it)
        {
          LRESULT res = SendMessage(server->StatusHWND, SWGC_ADD_SONG, (WPARAM)0, (LPARAM)*it);
        }
        break;
      default:
        break;
      }
    }

    if (InPlayback)
    {
      do
      {
        Sleep(100L);

        if (server->CheckPhrasePlaybackStatus() == true)
        {
          continue;
        }

        if (server->SoftStop == true)
        {
          server->ReleaseSoundBuffers();
          server->SoftStop = false; InPlayback = false;
          for (int x = 0; x < NUM_INSTRUMENTS; x++)
          {
            server->EnabledInstrument[x] = false;
          }
          continue;
        }

        int DoneCount = server->GetNextFlourish(&CurrFlo, &FlourishBuffer);

        if (DoneCount == NUM_INSTRUMENTS)
        {
          if (server->SofterStop == true)
          {
            server->ReleaseSoundBuffers();
            server->SofterStop = false; InPlayback = false;
            continue;
          }
        }

        server->PlayNextFlourish(CurrFlo);
        server->SendPlaylistUpdate(&FlourishBuffer, &CurrFlo);
      } while (bPendingSong && InPlayback);
    }
  }


  ExitThread(0);
}

void SWGPlayerServer::GenerateRandomSet(FlourishBufferType * FloBuff)
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


void SWGPlayerServer::InitFlourish(FlourishBufferType * FloBuff)
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

int SWGPlayerServer::InsertFlourish(FlourishBufferType * FloBuff, InstrumentSoundType Instrument, FlourishSoundType Flourish, bool FlushQueue)
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


int SWGPlayerServer::InsertBandFlourish(FlourishBufferType * FloBuff, FlourishSoundType Flourish, bool Final)
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

int SWGPlayerServer::GetNextFlourish(SoundListType * DestBuff, FlourishBufferType * FloBuff)
{

  int DoneCount = 0;
  bool AllowRandom = true;

  for (int x = 0; x < NUM_INSTRUMENTS; x++)
  {
    if (this->EnabledInstrument[x] == true)
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
        this->EnabledInstrument[x] = true;

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


  if (this->RandomizerEnabled && AllowRandom)
  {
    for (int x = 0; x < NUM_INSTRUMENTS; x++)
    {
      if (this->EnabledInstrument[x] == true)
      {
        DestBuff->SoundList[x] = FloBuff->RandBuffer.SoundList[x];
      }
    }
    GenerateRandomSet(FloBuff);
  }

  return DoneCount;
}

bool SWGPlayerServer::ReleaseSoundBuffers()
{
  int            NumInstruments = INSTRUMENT_SOUND_MAX;
  /*********************************
  Release Sound Buffers
  *********************************/
  for (int i = 0; i < NumInstruments; i++)
  {
    if (this->CurrentFlourish.SoundList[i] != -1)
    {
      if (this->DSBuffer[i] != NULL)
      {
        DSBuffer[i]->Release();
        DSBuffer[i] = NULL;
      }
    }
  }

  return true;
}

bool SWGPlayerServer::CreateDSoundObject()
{
  HRESULT        r;

  /*********************************
  Create DSound Interface Object
  *********************************/
  TRACE("DSOUND: Beginning Dsound...\n");

  r = DirectSoundCreate8(
    NULL, // Use default device
    &this->DSound,
    NULL
  );

  if (DS_OK != r)
  {
    TRACE("DSOUND: ERROR: Couldn't load DSound interface: %d\n", r);
    return false;
  }

  r = DSound->SetCooperativeLevel(this->ClientHWND, DSSCL_NORMAL);
  if (DS_OK != r)
  {
    TRACE("DSOUND: ERROR: Couldn't load DSound interface: %d\n", r);
    return false;
  }

  return true;
}

bool SWGPlayerServer::CheckPhrasePlaybackStatus()
{
  DWORD TempBuffStatus = 0;
  int   NumInstruments = INSTRUMENT_SOUND_MAX;
  int i;

  if (HardStop == true)
  {
    return false;
  }

  // Update BuffStatus
  for (i = 0; i < NumInstruments; i++)
  {
    if (this->CurrentFlourish.SoundList[i] != -1 && ((1 << i) & this->BuffStatus) == 0)
    {
      if (this->DSBuffer[i] != NULL)
      {
        this->DSBuffer[i]->GetStatus(&TempBuffStatus);
        this->BuffStatus |= (~TempBuffStatus & DSBSTATUS_PLAYING) << i;
        if ((~TempBuffStatus & DSBSTATUS_PLAYING) > 0)
        {
          this->DSBuffer[i]->Release();
          this->DSBuffer[i] = NULL;
        }
      }
    }
  }

  if (this->BuffStatus != this->InstrumentBitMask)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void SWGPlayerServer::PlayNextFlourish(SoundListType PhraseList)
{
  HRESULT        r;
  int   NumInstruments = INSTRUMENT_SOUND_MAX;
  int            i;

  for (i = 0; i < NUM_INSTRUMENTS; i++)
  {
    this->CurrentFlourish.SoundList[i] = PhraseList.SoundList[i];
  }

  /*********************************
  Play Buffer
  *********************************/
  this->BuffStatus = 0;
  this->InstrumentBitMask = 0;

  /*********************************
  Create Sound Buffer
  *********************************/
  for (i = 0; i < NumInstruments; i++)
  {
    if (this->CurrentFlourish.SoundList[i] != FLOURISH_SOUND_SILENCE
      && this->CurrentSong->dsBufferDesc((InstrumentSoundType)i, this->CurrentFlourish.SoundList[i])->dwBufferBytes > 0)
    {
      r = DSound->CreateSoundBuffer(
        this->CurrentSong->dsBufferDesc((InstrumentSoundType)i, this->CurrentFlourish.SoundList[i]),
        &this->DSBuffer[i],
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
    if (this->CurrentFlourish.SoundList[i] != FLOURISH_SOUND_SILENCE
      && this->CurrentSong->dsBufferDesc((InstrumentSoundType)i, this->CurrentFlourish.SoundList[i])->dwBufferBytes > 0)
    {
      void * FirstPage = NULL;
      DWORD  FirstPageSize = 0;
      void * SecondPage = NULL;
      DWORD  SecondPageSize = 0;
      DWORD  RemainingData = 0;

      r = this->DSBuffer[i]->Lock(
        0,
        this->CurrentSong->dsBufferDesc((InstrumentSoundType)i, this->CurrentFlourish.SoundList[i])->dwBufferBytes,
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

      memcpy(FirstPage, this->CurrentSong->data((InstrumentSoundType)i, this->CurrentFlourish.SoundList[i]), this->CurrentSong->dsBufferDesc((InstrumentSoundType)i, this->CurrentFlourish.SoundList[i])->dwBufferBytes);
      if (FirstPageSize <= this->CurrentSong->dsBufferDesc((InstrumentSoundType)i, this->CurrentFlourish.SoundList[i])->dwBufferBytes)
      {
        RemainingData = this->CurrentSong->dsBufferDesc((InstrumentSoundType)i, this->CurrentFlourish.SoundList[i])->dwBufferBytes - FirstPageSize;
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
          ((BYTE *)this->CurrentSong->data((InstrumentSoundType)i, this->CurrentFlourish.SoundList[i])) + FirstPageSize,
          RemainingData
        );
      }

      r = this->DSBuffer[i]->Unlock(
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
    if (this->CurrentFlourish.SoundList[i] != FLOURISH_SOUND_SILENCE
      && this->CurrentSong->dsBufferDesc((InstrumentSoundType)i, this->CurrentFlourish.SoundList[i])->dwBufferBytes > 0)
    {
      r = this->DSBuffer[i]->Play(0, 0, 0);

      if (DS_OK != r)
      {
        TRACE("DSOUND: ERROR: Couldn't play DSound buffer: %d\n", r);
        return;
      }
    }
  }

  this->InstrumentBitMask = 0;
  for (i = 0; i < NumInstruments; i++)
  {
    if (this->CurrentFlourish.SoundList[i] != FLOURISH_SOUND_SILENCE && this->EnabledInstrument[i] == true
      && this->CurrentSong->dsBufferDesc((InstrumentSoundType)i, this->CurrentFlourish.SoundList[i])->dwBufferBytes > 0)
    {
      this->InstrumentBitMask |= 1 << i;
    }
  }

}
