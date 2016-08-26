#pragma once
#include "stdafx.h"
#include <string>
#include "TREResource.h"
#include "ResourceRegistry.h"
#include <dsound.h>

typedef enum {
  FLOURISH_SOUND_SILENCE = -1,
  FLOURISH_SOUND_1 = 0,
  FLOURISH_SOUND_2,
  FLOURISH_SOUND_3,
  FLOURISH_SOUND_4,
  FLOURISH_SOUND_5,
  FLOURISH_SOUND_6,
  FLOURISH_SOUND_7,
  FLOURISH_SOUND_8,
  FLOURISH_SOUND_INTRO,
  FLOURISH_SOUND_IDLE,
  FLOURISH_SOUND_OUTRO,
  FLOURISH_SOUND_MAX,
  FLOURISH_SOUND_START,
  FLOURISH_SOUND_STOP
} FlourishSoundType;

#define NUM_FLOURISHES         FLOURISH_SOUND_MAX

class Flourish
{
protected:
  TREResource *resource;
  FlourishSoundType type;
  std::string resourceName;
  DSBUFFERDESC *dsBufferDesc;
  WAVEFORMATEX *wavFormat;
  void *data;

  std::string generateResourceNameVariant1(FlourishSoundType type, std::string prefix);
  std::string generateResourceNameVariant2(FlourishSoundType type, std::string prefix);

public:
  Flourish(FlourishSoundType type, ResourceRegistry *registry, std::string resourcePrefix);
  ~Flourish();
  BOOL prepare();
  
  DSBUFFERDESC *getDSBufferDesc();
  void *getData();

};
