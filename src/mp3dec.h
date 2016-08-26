#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <mmreg.h>


typedef struct
{
  unsigned char sync1;

  unsigned char protection:1;
  unsigned char layer:2;
  unsigned char version:2;
  unsigned char sync2:3;

  unsigned char private_bit:1;
  unsigned char padding_bit:1;
  unsigned char sampling_rate:2;
  unsigned char bitrate_idx:4;

  unsigned char emphasis:2;
  unsigned char original:1;
  unsigned char copyright:1;
  unsigned char mode_extension:2;
  unsigned char channel_mode:2;
} mp3header_format_type;



void convertMP3(
  char * cFileData,
  int size,
  LPWAVEFORMATEX waveFormat,
  int * total_o_size,
  char ** output_buff
);
