#include "stdafx.h"
#include "Debug.h"
#include "Song.h"


Song::Song(SongSoundType type, ResourceRegistry *registry)
{
  this->type = type;
  this->registry = registry;

  char prefix[] = "player_music/sample/song00";
  sprintf_s(prefix, sizeof(prefix), "player_music/sample/song%02d", (int)type);
  this->resourcePrefix.assign(prefix);

  for (int i = 0; i < INSTRUMENT_SOUND_MAX; i++)
  {
    this->instruments[i] = new Instrument((InstrumentSoundType)i, registry, this->resourcePrefix);
  }
}

Song::~Song()
{
  for (int i = 0; i < INSTRUMENT_SOUND_MAX; i++)
  {
    delete this->instruments[i];
  }
}

bool Song::prepare()
{
  bool success = false;

  for (int i = 0; i < INSTRUMENT_SOUND_MAX; i++)
  {
    this->instruments[i]->prepare();
  }

  return success;
}

void *Song::data(InstrumentSoundType instrument, FlourishSoundType flourish)
{
  return this->instruments[instrument]->data(flourish);
}

DSBUFFERDESC * Song::dsBufferDesc(InstrumentSoundType instrument, FlourishSoundType flourish)
{
  return this->instruments[instrument]->dsBufferDesc(flourish);
}
