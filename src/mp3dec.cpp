#include "stdafx.h"
#include "mp3dec.h"
#include <msacm.h>


int bitrate_idx[16][5] =
{
  {0, 0, 0, 0, 0},
  {32000, 32000, 32000, 32000, 8000},
  {64000, 48000, 40000, 48000, 16000},
  {96000, 56000, 48000, 56000, 24000},
  {128000,	64000,	56000,	64000,	32000},
  {160000,	80000,	64000,	80000,	40000},
  {192000,	96000,	80000,	96000,	48000},
  {224000,	112000,	96000,	112000,	56000},
  {256000,	128000,	112000,	128000,	64000},
  {288000,	160000,	128000,	144000,	80000},
  {320000,	192000,	160000,	160000,	96000},
  {352000,	224000,	192000,	176000,	112000},
  {384000,	256000,	224000,	192000,	128000},
  {416000,	320000,	256000,	224000,	144000},
  {448000,	384000,	320000,	256000,	160000},
  {0, 0, 0, 0, 0 }
};

int GetMP3Bitrate(int version, int layer, int idx)
{
  int bitrate = 0;
  switch( layer )
  {
  case 1: /* l3 */
    switch( version )
    {
    case 0: /* 2.5 */
    case 2: /* 2 */
      bitrate = bitrate_idx[idx][4];
      break;
    case 3: /* 1 */
      bitrate = bitrate_idx[idx][2];
      break;
    case 1: /* reserved */
    default:
      break;
    }
    break;
  case 2: /* l2 */
    switch( version )
    {
    case 0: /* 2.5 */
    case 2: /* 2 */
      bitrate = bitrate_idx[idx][4];
      break;
    case 3: /* 1 */
      bitrate = bitrate_idx[idx][1];
      break;
    case 1: /* reserved */
    default:
      break;
    }
    break;
  case 3: /* l1 */
    switch( version )
    {
    case 0: /* 2.5 */
    case 2: /* 2 */
      bitrate = bitrate_idx[idx][3];
      break;
    case 3: /* 1 */
      bitrate = bitrate_idx[idx][0];
      break;
    case 1: /* reserved */
    default:
      break;
    }
    break;
  }
  return bitrate;
}

int GetMP3SampFreq(int version, int rate)
{
  switch( version )
  {
  case 0: /* 2.5 */
    switch( rate )
    {
    case 0:
      return 11025;
    case 1:
      return 12000;
    case 2:
      return 8000;
    case 3:
    default:
      return 0;
      break;
    }
    break;
  case 2: /* 2 */
    switch( rate )
    {
    case 0:
      return 22050;
    case 1:
      return 24000;
    case 2:
      return 12000;
    case 3:
    default:
      return 0;
      break;
    }
    break;
  case 3: /* 1 */
    switch( rate )
    {
    case 0:
      return 44100;
    case 1:
      return 48000;
    case 2:
      return 32000;
    case 3:
    default:
      return 0;
      break;
    }
    break;
  case 1: /* reserved */
  default:
    return 0;
    break;
  }
}



int g_mp3Drivers = 0;

BOOL CALLBACK acmDriverEnumCallback( HACMDRIVERID hadid, DWORD dwInstance, DWORD fdwSupport ){
  if( fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC ) {
    MMRESULT mmr;

    ACMDRIVERDETAILS details;
    details.cbStruct = sizeof(ACMDRIVERDETAILS);
    mmr = acmDriverDetails( hadid, &details, 0 );

    HACMDRIVER driver;
    mmr = acmDriverOpen( &driver, hadid, 0 );

    unsigned int i;
    for(i = 0; i < details.cFormatTags; i++ ){
      ACMFORMATTAGDETAILS fmtDetails;
      ZeroMemory( &fmtDetails, sizeof(fmtDetails) );
      fmtDetails.cbStruct = sizeof(ACMFORMATTAGDETAILS);
      fmtDetails.dwFormatTagIndex = i;
      mmr = acmFormatTagDetails( driver, &fmtDetails, ACM_FORMATTAGDETAILSF_INDEX );
      if( fmtDetails.dwFormatTag == WAVE_FORMAT_MPEGLAYER3 ){
        OutputDebugString( "Found an MP3-capable ACM codec: " );
        OutputDebugString( details.szLongName );
        OutputDebugString( "\n" );
        g_mp3Drivers++;
      }
    }
    mmr = acmDriverClose( driver, 0 );
  }
  return true;
}

HACMSTREAM g_mp3stream = NULL;

void convertMP3(
  char * cFileData,
  int size,
  LPWAVEFORMATEX waveFormat,
  int * total_o_size,
  char ** output_buff
){
  
  int curr_ptr = 0;
  *total_o_size = 0;
  *output_buff = NULL;

// try to find an MP3 codec
  acmDriverEnum( acmDriverEnumCallback, 0, 0 );
  if(g_mp3Drivers == 0){
    OutputDebugString( "No MP3 decoders found!\n" );
    return;
  }

#ifdef _DEBUG
  static int fnum = 0;
  HANDLE hFile = NULL;
	BOOL           b_r = FALSE;
	DWORD          BytesWritten;
  char fname[255] = "";
  sprintf_s(fname,"sample%d.raw",fnum),

  hFile = CreateFile( 
    fname,
		GENERIC_WRITE, 
		FILE_SHARE_WRITE, 
		NULL, 
		CREATE_ALWAYS, 
		0, 
		NULL
		);
  fnum++;
#endif /* _DEBUG */

  ACMSTREAMHEADER mp3streamHead;
  ZeroMemory( &mp3streamHead, sizeof(ACMSTREAMHEADER ) );
  bool prepared = false;

  while( size - curr_ptr > 10 )
  {
    MMRESULT mmr;
    MPEGLAYER3WAVEFORMAT mp3format;
    mp3header_format_type * mp3header;

    mp3header = (mp3header_format_type *)cFileData;

    if( mp3header->sync1 != 0xff
     || mp3header->sync2 != 0x7 )
    {
      cFileData -= 1;
      curr_ptr -= 1;
    }

    mp3header = (mp3header_format_type *)cFileData;

    while( ( mp3header->sync1 != 0xff
          || mp3header->sync2 != 0x7 )
     && size - curr_ptr > 10)
    {
      cFileData += 1;
      curr_ptr -= 1;
      mp3header = (mp3header_format_type *)cFileData;
    }

    mp3format.fdwFlags            = MPEGLAYER3_FLAG_PADDING_OFF;
    mp3format.nCodecDelay         = 1393;
    mp3format.nFramesPerBlock     = 1;
    mp3format.wfx.cbSize          = MPEGLAYER3_WFX_EXTRA_BYTES;
    mp3format.wfx.nAvgBytesPerSec = GetMP3Bitrate(mp3header->version, mp3header->layer, mp3header->bitrate_idx) / 8;
    mp3format.wfx.nBlockAlign     = 1;
    mp3format.wfx.nChannels       = (mp3header->channel_mode == 3 ? 1 : 2);
    mp3format.wfx.nSamplesPerSec  = GetMP3SampFreq(mp3header->version, mp3header->sampling_rate);
    mp3format.wfx.wBitsPerSample  = 0;
    mp3format.wfx.wFormatTag      = WAVE_FORMAT_MPEGLAYER3;
    mp3format.wID                 = MPEGLAYER3_ID_MPEG;

    mp3format.nBlockSize          = (mp3header->layer == 3 ? 
      ((12 * GetMP3Bitrate(mp3header->version, mp3header->layer, mp3header->bitrate_idx) / mp3format.wfx.nSamplesPerSec + mp3header->padding_bit) * 4) : 
      (144 * GetMP3Bitrate(mp3header->version, mp3header->layer, mp3header->bitrate_idx) / mp3format.wfx.nSamplesPerSec + mp3header->padding_bit) );


      
      
    // define desired output format
    waveFormat->wFormatTag = WAVE_FORMAT_PCM;
    waveFormat->nChannels = mp3format.wfx.nChannels;
    waveFormat->nSamplesPerSec = mp3format.wfx.nSamplesPerSec;
    waveFormat->wBitsPerSample = 16;
    waveFormat->nBlockAlign = 2 * waveFormat->nChannels;
    waveFormat->nAvgBytesPerSec = waveFormat->nBlockAlign * waveFormat->nSamplesPerSec;
    waveFormat->cbSize = 0;


    // find the biggest format size
    DWORD maxFormatSize = 0;
    mmr = acmMetrics( NULL, ACM_METRIC_MAX_SIZE_FORMAT, &maxFormatSize );
    
    
    
    // define MP3 input format
/*
    LPMPEGLAYER3WAVEFORMAT mp3format = (LPMPEGLAYER3WAVEFORMAT) LocalAlloc( LPTR, maxFormatSize );
    mp3format->wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
    mp3format->wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
    mp3format->wfx.nChannels = 2;
    mp3format->wfx.nAvgBytesPerSec = 128 * (1024 / 8);  // not really used but must be one of 64, 96, 112, 128, 160kbps
    mp3format->wfx.wBitsPerSample = 0;                  // MUST BE ZERO
    mp3format->wfx.nBlockAlign = 1;                     // MUST BE ONE
    mp3format->wfx.nSamplesPerSec = 44100;              // 44.1kHz
    mp3format->fdwFlags = MPEGLAYER3_FLAG_PADDING_OFF;
    mp3format->nBlockSize = MP3_BLOCK_SIZE;             // voodoo value #1
    mp3format->nFramesPerBlock = 1;                     // MUST BE ONE
    mp3format->nCodecDelay = 1393;                      // voodoo value #2
    mp3format->wID = MPEGLAYER3_ID_MPEG;
*/

    g_mp3stream = NULL;
    mmr = acmStreamOpen( &g_mp3stream,               // open an ACM conversion stream
  		      NULL,                       // querying all ACM drivers
  		      (LPWAVEFORMATEX) &mp3format, // converting from MP3
  		      waveFormat,                 // to WAV
  		      NULL,                       // with no filter
  		      0,                          // or async callbacks
  		      0,                          // (and no data for the callback)
  		      0                           // and no flags
  		      );
    
    switch( mmr ) {
    case MMSYSERR_NOERROR:
      break; // success!
    case MMSYSERR_INVALPARAM:
      assert( !"Invalid parameters passed to acmStreamOpen" );
      return;
    case ACMERR_NOTPOSSIBLE:
      assert( !"No ACM filter found capable of decoding MP3" );
      return;
    default:
      assert( !"Some error opening ACM decoding stream!" );
      return;
    }
    
    // MP3 stream converter opened correctly
    // now, let's open a file, read in a bunch of MP3 data, and convert it!
    
    // find out how big the decompressed buffer will be
    unsigned long rawbufsize = 0;
    mmr = acmStreamSize( g_mp3stream, size /*mp3format.nBlockSize*/, &rawbufsize, ACM_STREAMSIZEF_SOURCE );
    assert( mmr == 0 );
    assert( rawbufsize > 0 );
    
    // allocate our I/O buffers
    LPBYTE rawbuf = (LPBYTE) LocalAlloc( LPTR, rawbufsize );
    
    // prepare the decoder
    mp3streamHead.cbStruct = sizeof(ACMSTREAMHEADER );
    mp3streamHead.pbSrc = (LPBYTE)cFileData;
    mp3streamHead.cbSrcLength = size; //mp3format.nBlockSize;
    mp3streamHead.pbDst = rawbuf;
    mp3streamHead.cbDstLength = rawbufsize;

    if( !prepared )
    {
      mmr = acmStreamPrepareHeader( g_mp3stream, &mp3streamHead, 0 );
      assert( mmr == 0 );
      prepared = true;
    }
    
    // convert the data
    mmr = acmStreamConvert( g_mp3stream, &mp3streamHead, ACM_STREAMCONVERTF_BLOCKALIGN );
    assert( mmr == 0 );
    
    
    if( *output_buff == NULL )
    {
      *output_buff = reinterpret_cast<char *>(HeapAlloc(GetProcessHeap(),0,mp3streamHead.cbDstLengthUsed));
    }
    else
    {
      *output_buff = reinterpret_cast<char *>(HeapReAlloc(GetProcessHeap(),0,*output_buff,(*total_o_size)+mp3streamHead.cbDstLengthUsed));
    }

    assert( output_buff != NULL );

    memcpy(&(*output_buff)[*total_o_size],rawbuf,mp3streamHead.cbDstLengthUsed);


#ifdef _DEBUG
    b_r = WriteFile(
      hFile,
      rawbuf,
      mp3streamHead.cbDstLengthUsed,
      &BytesWritten,
      NULL
      );
#endif /* _DEBUG */

    *total_o_size += mp3streamHead.cbDstLengthUsed;

    mmr = acmStreamUnprepareHeader( g_mp3stream, &mp3streamHead, 0 );
    assert( mmr == 0 );
    LocalFree(rawbuf);
    mmr = acmStreamClose( g_mp3stream, 0 );
    assert( mmr == 0 );

    break;
    curr_ptr += mp3format.nBlockSize;
    cFileData += mp3format.nBlockSize;

  }

#ifdef _DEBUG
  CloseHandle(hFile);
#endif /* _DEBUG */
}
