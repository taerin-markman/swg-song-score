#include "stdafx.h"
#include "SWGPlayer.h"
#include "Debug.h"
#include "ResourceRegistry.h"

SWGPlayer::SWGPlayer(HWND hWnd)
{
  this->registry = new ResourceRegistry();
  this->server = new SWGPlayerServer(hWnd, this->registry);
}

bool SWGPlayer::EnableRandomFlourish(bool enable)
{
  if (true == this->server->SendCommand(
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

bool SWGPlayer::SetStatusReceiverHandle(HWND hwnd)
{
  if (true == this->server->SendCommand(
    SWG_PLAYER_CMD_SET_DIALOG_HANDLE,
    reinterpret_cast<DWORD>(hwnd),
    0
  )
    )
  {
    return true;
  }
  return false;
}


bool SWGPlayer::Flourish(InstrumentSoundType Instrument, FlourishSoundType Flourish)
{
  if (Flourish < FLOURISH_SOUND_MAX)
  {
    if (true == this->server->SendCommand(
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

bool SWGPlayer::BandFlourish(FlourishSoundType Flourish)
{
  if (Flourish < FLOURISH_SOUND_MAX)
  {
    if (true == this->server->SendCommand(
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

bool SWGPlayer::ChangeMusic(SongSoundType SongNum)
{
  if (SongNum < SONG_SOUND_MAX)
  {
    if (true == this->server->SendCommand(
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

bool SWGPlayer::Play(SongSoundType SongNum, FlourishSoundType FlourishNum)
{
  if (SongNum < SONG_SOUND_MAX && FlourishNum < FLOURISH_SOUND_MAX)
  {
    if (true == this->server->SendCommand(
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

bool SWGPlayer::EnableInstrument(InstrumentSoundType Inst, bool enable, SWGPlayerStopType stop)
{
  if (Inst < INSTRUMENT_SOUND_MAX)
  {
    if (enable == true)
    {
      if (true == this->server->SendCommand(
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
      if (true == this->server->SendCommand(
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

bool SWGPlayer::Stop(SWGPlayerStopType StopType)
{
  if (StopType < SWG_PLAYER_STOP_MAX)
  {
    if (true == this->server->SendCommand(
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





