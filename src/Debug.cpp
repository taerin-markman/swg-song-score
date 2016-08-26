#include "stdafx.h"
#include "Debug.h"
#include "SWG Player.h"
#include "TREFile.h"

#ifdef _DEBUG
void DebugMessage(const char * const file, int line, const char * const text)
{
  char fmt[] = "%s %d: %s\n";
  char i[255];
  _snprintf_s(i, 254, fmt, file, line, text);
  OutputDebugString(i);
}
#endif

#if 0
bool SWGPlayer(HWND hWnd, HANDLE * SWGPlayerH)
{
  TREFile *treFile = new TREFile("data_sample_00.tre");
  treFile->load();
  return false;
}

bool EnableInstrument(InstrumentSoundType Inst, bool enable, SWGPlayerStopType stop)
{
  return false;
}


bool Flourish(InstrumentSoundType Instrument, FlourishSoundType Flourish)
{
  return false;
}


bool BandFlourish(FlourishSoundType Flourish)
{
  return false;
}


bool ChangeMusic(SongSoundType SongNum)
{
  return false;
}


bool Play(SongSoundType SongNum, FlourishSoundType FlourishNum)
{
  return false;
}


bool Stop(SWGPlayerStopType StopType)
{
  return false;
}


bool EnableRandomFlourish(bool enable)
{
  return false;
}


bool SWGSetDialog(HWND hDlg)
{
  return false;
}

#endif
