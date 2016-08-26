#pragma once
#include "stdafx.h"
#include <string>
#include "Flourish.h"
#include "ResourceRegistry.h"

typedef enum {
  INSTRUMENT_SOUND_BANDFILL = 0,
  INSTRUMENT_SOUND_CHIDINKALU,
  INSTRUMENT_SOUND_MANDOVIOL,
  INSTRUMENT_SOUND_NALARGON,
  INSTRUMENT_SOUND_TRAZ,
  INSTRUMENT_SOUND_XANTHA,
  INSTRUMENT_SOUND_MAX
} InstrumentSoundType;

#define NUM_INSTRUMENTS        INSTRUMENT_SOUND_MAX

#define NUM_PHRASES            (NUM_FLOURISHES * NUM_INSTRUMENTS)

class Instrument
{
protected:
  InstrumentSoundType type;
  Flourish *flourishes[FLOURISH_SOUND_MAX];
  std::string resourcePrefix;
  ResourceRegistry *registry;

public:

  Instrument(InstrumentSoundType type, ResourceRegistry *registry, std::string resourcePrefix);
  ~Instrument();
  BOOL prepare();
  void *data(FlourishSoundType flourish);
  DSBUFFERDESC *dsBufferDesc(FlourishSoundType flourish);
};
