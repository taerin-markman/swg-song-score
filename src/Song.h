#pragma once
#include "stdafx.h"
#include <vector>
#include "ResourceRegistry.h"
#include "Instrument.h"

typedef enum {
  SONG_SOUND_ROCK = 1,
  SONG_SOUND_JAZZ,
  SONG_SOUND_VIRTUOSO,
  SONG_SOUND_STARWARS1,
  SONG_SOUND_STARWARS2,
  SONG_SOUND_STARWARS3,
  SONG_SOUND_FOLK,
  SONG_SOUND_BALLAD,
  SONG_SOUND_CEREMONIAL,
  SONG_SOUND_WALTZ,
  SONG_SOUND_SWING = 11,
  SONG_SOUND_FUNK,
  SONG_SOUND_CARNIVAL,
  SONG_SOUND_CALYPSO,
  SONG_SOUND_ZYDECO = 15,
  SONG_SOUND_WESTERN = 16,
  SONG_SOUND_POP = 18,
  SONG_SOUND_STARWARS4 = 19,
  SONG_SOUND_BOOGIE,
  SONG_SOUND_MAX
} SongSoundEnumType;

typedef unsigned int SongSoundType;

#define NUM_SONGS              SONG_SOUND_MAX

class Song
{
protected:
  SongSoundType type;
  ResourceRegistry *registry;
  Instrument *instruments[NUM_INSTRUMENTS];
  std::string resourcePrefix;

public:
  Song(SongSoundType type, ResourceRegistry *registry);
  ~Song();
  bool prepare();
  void *data(InstrumentSoundType instrument, FlourishSoundType flourish);
  DSBUFFERDESC *dsBufferDesc(InstrumentSoundType instrument, FlourishSoundType flourish);
};
