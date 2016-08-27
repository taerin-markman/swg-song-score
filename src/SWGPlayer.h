#pragma once
#include "stdafx.h"
#include "SWGPlayerServer.h"

class SWGPlayer 
{
protected:
  SWGPlayerServer *server;
  ResourceRegistry *registry;

public:
	SWGPlayer(HWND hWnd);
	bool Play(SongSoundType SongNum, FlourishSoundType FlourishNum);
	bool Stop(SWGPlayerStopType StopType);
	bool Flourish(InstrumentSoundType Instrument, FlourishSoundType Flourish);
  bool BandFlourish(FlourishSoundType Flourish);
  bool ChangeMusic(SongSoundType SongNum);
  bool EnableInstrument(InstrumentSoundType Inst, bool enable, SWGPlayerStopType stop = SWG_PLAYER_STOP_END_OF_PHRASE);
  bool EnableRandomFlourish(bool enable);
  bool SetStatusReceiverHandle(HWND hwnd);
};
