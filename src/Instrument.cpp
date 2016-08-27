#include "stdafx.h"
#include "Instrument.h"
#include "Debug.h"

Instrument::Instrument(InstrumentSoundType type, ResourceRegistry *registry, std::string resourcePrefix)
{
  this->registry = registry;
  switch (type)
  {
  case INSTRUMENT_SOUND_MANDOVIOL:
    this->resourcePrefix = resourcePrefix + "_mand";
    break;
  case INSTRUMENT_SOUND_CHIDINKALU:
    this->resourcePrefix = resourcePrefix + "_khorn";
    break;
  case INSTRUMENT_SOUND_TRAZ:
    this->resourcePrefix = resourcePrefix + "_shorn";
    break;
  case INSTRUMENT_SOUND_NALARGON:
    this->resourcePrefix = resourcePrefix + "_nlrg";
    break;
  case INSTRUMENT_SOUND_BANDFILL:
    this->resourcePrefix = resourcePrefix + "_drum";
    break;
  case INSTRUMENT_SOUND_XANTHA:
    this->resourcePrefix = resourcePrefix + "_xantha";
    break;
  default:
    TRACE("Bad instrument value: %d", type);
    break;
  }
  
  this->type = type;

  for (int i = 0; i < FLOURISH_SOUND_MAX; i++)
  {
    this->flourishes[i] = new Flourish((FlourishSoundType)i, registry, this->resourcePrefix);
  }

}

Instrument::~Instrument()
{
  for (int i = 0; i < FLOURISH_SOUND_MAX; i++)
  {
    delete this->flourishes[i];
  }
}

bool Instrument::prepare()
{
  bool success = false;

  for (int i = 0; i < FLOURISH_SOUND_MAX; i++)
  {
    this->flourishes[i]->prepare();
  }

  return success;
}

void *Instrument::data(FlourishSoundType flourish)
{
  return this->flourishes[flourish]->getData();
}

DSBUFFERDESC * Instrument::dsBufferDesc(FlourishSoundType flourish)
{
  return this->flourishes[flourish]->getDSBufferDesc();
}
