#pragma once

#include "Song.h"

#ifdef _DEBUG
#define FEATURE_PLAYER_CLASS
#endif

#define MAX_FLOURISH_BUFFERS 6

#define SWGC_STATUS (WM_USER+1)

typedef enum {
	SWG_PLAYER_MSG_DONE,
	SWG_PLAYER_MSG_MAX
} SWGPlayerMessageType;

typedef enum {
	SWG_PLAYER_STOP_GRACEFUL,
	SWG_PLAYER_STOP_END_OF_PHRASE,
	SWG_PLAYER_STOP_NOW,
	SWG_PLAYER_STOP_MAX
} SWGPlayerStopType;

typedef struct {
	FlourishSoundType SoundList[NUM_INSTRUMENTS];
} SoundListType;

//typedef void (CALLBACK *SWGPlayerCallbackType)( SWGPlayerMessageType Message );

typedef struct {
	HWND                  m_hWnd;
	SoundListType *       PhraseList;
	int                   NumListElements;
	int                   SongNum;
} PhraseListParamsType;

typedef struct {
	SoundListType Buffer[MAX_FLOURISH_BUFFERS];
	SoundListType BandBuffer;
	SoundListType RandBuffer;
	bool          BandFlo;
	int           CurrFloIndex[NUM_INSTRUMENTS];
	int           UsedFloBuffs[NUM_INSTRUMENTS];
	int           CurrInsertLoc[NUM_INSTRUMENTS];
} FlourishBufferType;

bool SWGPlayer(HWND hWnd, HANDLE * SWGPlayerH);

bool EnableInstrument(InstrumentSoundType Inst, bool enable, SWGPlayerStopType stop = SWG_PLAYER_STOP_END_OF_PHRASE);
bool Flourish(InstrumentSoundType Instrument, FlourishSoundType Flourish);
bool BandFlourish(FlourishSoundType Flourish);
bool ChangeMusic(SongSoundType SongNum);
bool Play(SongSoundType SongNum, FlourishSoundType FlourishNum);
bool Stop(SWGPlayerStopType StopType);
bool EnableRandomFlourish(bool enable);
bool SWGSetDialog(HWND hDlg);


#ifdef FEATURE_PLAYER_CLASS
class SWGPlayer 
{
protected:
  bool HardStop, SoftStop, SofterStop;
	DWORD player_hWnd;
	HWND client_hWnd;
	HANDLE ThreadHandle;
	PhraseListParamsType PhraseListParams;
	void PlayPhraseList(HWND m_hWnd, SoundListType * PhraseList, int NumListElements, int SongNum);

public:
	SWGPlayer(HWND hWnd);
	DWORD WINAPI PlayPhraseListProc(LPVOID lpParameter);
	void Play(SongSoundType SongNum);
	void Stop(SWGPlayerStopType StopType);
	void Flourish(SoundListType * PhraseList);
	void ChangeMusic(SongSoundType SongNum);
};
#endif /* FEATURE_PLAYER_CLASS */