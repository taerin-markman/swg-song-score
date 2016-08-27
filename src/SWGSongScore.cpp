// SWGSongScore.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <stdio.h>
#include "SWGSongScore.h"
#define MAX_LOADSTRING 100

#include "SWGPlayer.h"
#include "Debug.h"
#include "Song.h"

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

HWND hWndMain, hWndDialog;
HANDLE SWGPlayerHandle;
SWGPlayer *player;

typedef struct {
  HWND hwnd;
  unsigned int song_id;
  char name[20];
} SWGWinSongButtonType;

#define MAX_SONG_BUTTONS 50
SWGWinSongButtonType SongButtons[MAX_SONG_BUTTONS] = { 0 };
unsigned int num_songs = 0;

SongSoundType SelectedSong = SONG_SOUND_STARWARS1;
InstrumentSoundType SelectedInstrument = INSTRUMENT_SOUND_BANDFILL;

const int ActiveInstControls[] =
{
/* INSTRUMENT_SOUND_BANDFILL   */ IDC_INST_OMMNIBOX,
/* INSTRUMENT_SOUND_CHIDINKALU */ IDC_INST_CHIDINKALU,
/* INSTRUMENT_SOUND_MANDOVIOL  */ IDC_INST_MANDOVIOL,
/* INSTRUMENT_SOUND_NALARGON   */ IDC_INST_NALARGON,
/* INSTRUMENT_SOUND_TRAZ       */ IDC_INST_TRAZ,
/* INSTRUMENT_SOUND_XANTHA     */ IDC_INST_XANTHA
};

const int InstrumentStatusControls[] =
{
/* INSTRUMENT_SOUND_BANDFILL   */ IDC_STATUS_O,
/* INSTRUMENT_SOUND_CHIDINKALU */ IDC_STATUS_C,
/* INSTRUMENT_SOUND_MANDOVIOL  */ IDC_STATUS_M,
/* INSTRUMENT_SOUND_NALARGON   */ IDC_STATUS_N,
/* INSTRUMENT_SOUND_TRAZ       */ IDC_STATUS_T,
/* INSTRUMENT_SOUND_XANTHA     */ IDC_STATUS_X
};


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
bool				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	MainDialog(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SWGSONGSCOREWIN32, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_SWGSONGSCOREWIN32);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(hWndDialog, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_SWGSONGSCOREWIN32);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_SWGSONGSCOREWIN32;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(hInstance, (LPCTSTR)IDI_SWGSONGSCOREWIN32); //LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
bool InitInstance(HINSTANCE hInstance, int nCmdShow)
{
  hInst = hInstance; // Store instance handle in our global variable

  hWndMain = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

  if (!hWndMain)
  {
    return FALSE;
  }

  player = new SWGPlayer(hWndMain);
	if (!player)
	{
		MessageBoxEx(hWndMain,"ERROR: Unable to find Star Wars Galaxies files. Make sure this program is run from the Star Wars Galaxies installation directory.","Unable to Start",MB_ICONSTOP | MB_OK,NULL);
		return FALSE;
	}

	hWndDialog = CreateDialogParam(hInst, (LPCTSTR)IDD_MAINDIALOG, hWndMain, (DLGPROC)MainDialog, 0);
	if (!hWndDialog)
	{
		TRACE("Can't create dialog: %d",GetLastError());
		return FALSE;
	}

  player->SetStatusReceiverHandle(hWndDialog);

  return TRUE;
}

void AddSong(SongSoundType song_id)
{
  char * name = "(Unknown)";
  switch( song_id )
  {
	case SONG_SOUND_ROCK:
    name = "Rock";
    break;
	case SONG_SOUND_JAZZ:
    name = "Jazz";
    break;
	case SONG_SOUND_VIRTUOSO:
    name = "Virtuoso";
    break;
	case SONG_SOUND_STARWARS1:
    name = "Star Wars 1";
    break;
	case SONG_SOUND_STARWARS2:
    name = "Star Wars 2";
    break;
	case SONG_SOUND_STARWARS3:
    name = "Star Wars 3";
    break;
	case SONG_SOUND_FOLK:
    name = "Folk";
    break;
	case SONG_SOUND_BALLAD:
    name = "Ballad";
    break;
	case SONG_SOUND_CEREMONIAL:
    name = "Ceremonial";
    break;
	case SONG_SOUND_WALTZ:
    name = "Waltz";
    break;
  case SONG_SOUND_SWING:
    name = "Swing";
    break;
  case SONG_SOUND_FUNK:
    name = "Funk";
    break;
  case SONG_SOUND_CALYPSO:
    name = "Calypso";
    break;
  case SONG_SOUND_WESTERN:
    name = "Western";
    break;
  case SONG_SOUND_STARWARS4:
    name = "Star Wars 4";
    break;
  case SONG_SOUND_BOOGIE:
    name = "Boogie";
    break;
  case SONG_SOUND_CARNIVAL:
    name = "Carnival";
    break;
  case SONG_SOUND_ZYDECO:
    name = "Zydeco";
    break;
  case SONG_SOUND_POP:
    name = "Pop";
    break;
  default:
    break;
  }

  SongButtons[num_songs].song_id = song_id;
  sprintf_s(SongButtons[num_songs].name,"%d. %s",song_id,name);
  num_songs++;
}

void SortSongs()
{
  bool nochange = false;
  unsigned int x = 0;

  while( !nochange )
  {
    nochange = true;
    for( x = 0; x < num_songs-1 && num_songs > 0; x++ )
    {
      if( SongButtons[x].song_id > SongButtons[x+1].song_id )
      {
        char name[20];
        unsigned int song_id = SongButtons[x].song_id;
        HWND hwnd = SongButtons[x].hwnd;

        strcpy_s(name,SongButtons[x].name);
        nochange = false;
        
        strcpy_s(SongButtons[x].name,SongButtons[x+1].name);
        SongButtons[x].song_id = SongButtons[x+1].song_id;
        SongButtons[x].hwnd = SongButtons[x+1].hwnd;

        strcpy_s(SongButtons[x+1].name,name);
        SongButtons[x+1].song_id = song_id;
        SongButtons[x+1].hwnd = hwnd;
      }
    }
  }
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) 
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void EnumerateSongList(HWND hDlg)
{
  bool first_song = true;

  unsigned int xpos;
  unsigned int ypos;
  xpos = 372;
  ypos = 63;

  SortSongs();
  for (unsigned int x = 0; x < num_songs; x++)
  {
    if (SongButtons[x].hwnd)
    {
      DestroyWindow(SongButtons[x].hwnd);
      SongButtons[x].hwnd = NULL;
    }
  }

  for (unsigned int x = 0; x < num_songs; x++)
  {
    HFONT hfnt, hOldFont;
    HDC hdc;
    DWORD winstyle = 0;

    if (first_song)
    {
      winstyle = WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP;
    }
    else
    {
      winstyle = WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON;
    }

    SongButtons[x].hwnd = CreateWindow(
      "BUTTON",   // predefined class 
      SongButtons[x].name,       // button text 
      winstyle,  // styles 
      xpos,         // starting x position 
      ypos,         // starting y position 
      100,        // button width 
      15,        // button height 
      hDlg,       // parent window 
      NULL,       // No menu 
      (HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE),
      NULL);      // pointer not needed

    hdc = GetDC(SongButtons[x].hwnd);

    hfnt = (HFONT)GetStockObject(ANSI_VAR_FONT);
    if (hOldFont = (HFONT)SelectObject(hdc, hfnt))
    {
      SendMessage(SongButtons[x].hwnd, WM_SETFONT, (WPARAM)hfnt, (LPARAM)true);
      SelectObject(hdc, hOldFont);
    }

    ypos += 15;
    if (x > 0 && x % 13 == 0)
    {
      ypos = 63;
      xpos += 100;
    }

    if (first_song)
    {
      first_song = false;
      SelectedSong = SongButtons[x].song_id;
      SendMessage(SongButtons[x].hwnd, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
    }

    hdc = GetDC(GetDlgItem(hDlg, IDC_STATUS_TITLE));
    hfnt = (HFONT)GetStockObject(ANSI_FIXED_FONT);

    if (hOldFont = (HFONT)SelectObject(hdc, hfnt))
    {
      SendMessage(GetDlgItem(hDlg, IDC_STATUS_TITLE), WM_SETFONT, (WPARAM)hfnt, (LPARAM)true);
      SendMessage(GetDlgItem(hDlg, IDC_STATUS_TITLE), WM_SETTEXT, (WPARAM)0, (LPARAM) "playing <-- queued");
    }
    SelectObject(hdc, hOldFont);
  }
}

LRESULT CALLBACK MainDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  bool retval = false;
	HICON hIcon; 

	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hDlg,IDC_VERSION_TEXT,"v0.9.3.3");

		hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_SWGSONGSCOREWIN32));

		SendMessage(hDlg,WM_SETICON,(WPARAM)ICON_BIG,(LPARAM)hIcon);
		SendMessage(hDlg,WM_SETICON,(WPARAM)ICON_SMALL,(LPARAM)hIcon);

		for (int x=0; x<(sizeof(ActiveInstControls)/sizeof(int)); x++)
		{
			CheckDlgButton(hDlg,ActiveInstControls[x],BST_CHECKED);
		}
		EnableWindow(GetDlgItem(hDlg,IDC_INST_OMMNIBOX),false);
		CheckRadioButton(hDlg,IDC_CURRINST_OMMNIBOX,IDC_CURRINST_XANTHA,IDC_CURRINST_OMMNIBOX);

    return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
    case 0:
      for( unsigned int x = 0; x < num_songs; x++ )
      {
        if( SongButtons[x].hwnd == (HWND)lParam )
        {
          SelectedSong = SongButtons[x].song_id;
          TRACE("song: %d",SelectedSong);
        }
      }
      break;

		case IDOK:
			player->Stop(SWG_PLAYER_STOP_NOW);
			WaitForSingleObject(SWGPlayerHandle,100L);
			CloseHandle(SWGPlayerHandle);
			EndDialog(hDlg, LOWORD(wParam));
			DestroyWindow(hWndMain);
			return TRUE;

		case IDC_FLOURISH1:
      player->Flourish( SelectedInstrument, FLOURISH_SOUND_1 );
			break;
		case IDC_FLOURISH2:
      player->Flourish( SelectedInstrument, FLOURISH_SOUND_2 );
			break;
		case IDC_FLOURISH3:
      player->Flourish( SelectedInstrument, FLOURISH_SOUND_3 );
			break;
		case IDC_FLOURISH4:
      player->Flourish( SelectedInstrument, FLOURISH_SOUND_4 );
			break;
		case IDC_FLOURISH5:
      player->Flourish( SelectedInstrument, FLOURISH_SOUND_5 );
			break;
		case IDC_FLOURISH6:
      player->Flourish( SelectedInstrument, FLOURISH_SOUND_6 );
			break;
		case IDC_FLOURISH7:
      player->Flourish( SelectedInstrument, FLOURISH_SOUND_7 );
			break;
		case IDC_FLOURISH8:
      player->Flourish( SelectedInstrument, FLOURISH_SOUND_8 );
			break;
		case IDC_FLOURISH_SILENCE:
      player->Flourish( SelectedInstrument, FLOURISH_SOUND_SILENCE );
			break;

		case IDC_BAND_FLOURISH1:
      player->BandFlourish( FLOURISH_SOUND_1 );
			break;
		case IDC_BAND_FLOURISH2:
      player->BandFlourish( FLOURISH_SOUND_2 );
			break;
		case IDC_BAND_FLOURISH3:
      player->BandFlourish( FLOURISH_SOUND_3 );
			break;
		case IDC_BAND_FLOURISH4:
      player->BandFlourish( FLOURISH_SOUND_4 );
			break;
		case IDC_BAND_FLOURISH5:
      player->BandFlourish( FLOURISH_SOUND_5 );
			break;
		case IDC_BAND_FLOURISH6:
      player->BandFlourish( FLOURISH_SOUND_6 );
			break;
		case IDC_BAND_FLOURISH7:
      player->BandFlourish( FLOURISH_SOUND_7 );
			break;
		case IDC_BAND_FLOURISH8:
      player->BandFlourish( FLOURISH_SOUND_8 );
			break;

		case IDC_PLAY:
      player->Play( SelectedSong, FLOURISH_SOUND_INTRO );
			EnableWindow(GetDlgItem(hDlg,IDC_PLAY), false);
			EnableWindow(GetDlgItem(hDlg,IDC_HARDSTOP), true);
			EnableWindow(GetDlgItem(hDlg,IDC_SOFTSTOP), true);
			EnableWindow(GetDlgItem(hDlg,IDC_SOFTERSTOP), true);
			EnableWindow(GetDlgItem(hDlg,IDC_CHANGE_MUSIC), true);
			break;

		case IDC_CHANGE_MUSIC:
      player->ChangeMusic( SelectedSong );
			break;

    case IDC_HARDSTOP:
      player->Stop( SWG_PLAYER_STOP_NOW );
			EnableWindow(GetDlgItem(hDlg,IDC_PLAY), true);
			EnableWindow(GetDlgItem(hDlg,IDC_HARDSTOP), false);
			EnableWindow(GetDlgItem(hDlg,IDC_SOFTSTOP), false);
			EnableWindow(GetDlgItem(hDlg,IDC_SOFTERSTOP), false);
			EnableWindow(GetDlgItem(hDlg,IDC_CHANGE_MUSIC), false);
			break;

		case IDC_SOFTSTOP:
      player->Stop( SWG_PLAYER_STOP_END_OF_PHRASE );
			EnableWindow(GetDlgItem(hDlg,IDC_PLAY), true);
			EnableWindow(GetDlgItem(hDlg,IDC_HARDSTOP), false);
			EnableWindow(GetDlgItem(hDlg,IDC_SOFTSTOP), false);
			EnableWindow(GetDlgItem(hDlg,IDC_SOFTERSTOP), false);
			EnableWindow(GetDlgItem(hDlg,IDC_CHANGE_MUSIC), false);
			break;

		case IDC_SOFTERSTOP:
      player->Stop( SWG_PLAYER_STOP_GRACEFUL );
			EnableWindow(GetDlgItem(hDlg,IDC_PLAY), true);
			EnableWindow(GetDlgItem(hDlg,IDC_HARDSTOP), false);
			EnableWindow(GetDlgItem(hDlg,IDC_SOFTSTOP), false);
			EnableWindow(GetDlgItem(hDlg,IDC_SOFTERSTOP), false);
			EnableWindow(GetDlgItem(hDlg,IDC_CHANGE_MUSIC), false);
			break;

		case IDC_STOP_SINGLE:
      player->EnableInstrument(
				SelectedInstrument, 
				false,
				SWG_PLAYER_STOP_GRACEFUL
				);
			CheckDlgButton(hDlg,ActiveInstControls[SelectedInstrument],BST_UNCHECKED);
			break;


		case IDC_INST_OMMNIBOX:
      player->EnableInstrument(
				INSTRUMENT_SOUND_BANDFILL, 
				(BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_INST_OMMNIBOX) ? true : false) 
				);
			break;
		case IDC_INST_TRAZ:
      player->EnableInstrument(
				INSTRUMENT_SOUND_TRAZ, 
				(BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_INST_TRAZ) ? true : false) 
				);
			break;
		case IDC_INST_CHIDINKALU:
      player->EnableInstrument(
				INSTRUMENT_SOUND_CHIDINKALU, 
				(BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_INST_CHIDINKALU) ? true : false) 
				);
			break;
		case IDC_INST_NALARGON:
      player->EnableInstrument(
				INSTRUMENT_SOUND_NALARGON, 
				(BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_INST_NALARGON) ? true : false) 
				);
			break;
		case IDC_INST_MANDOVIOL:
      player->EnableInstrument(
				INSTRUMENT_SOUND_MANDOVIOL, 
				(BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_INST_MANDOVIOL) ? true : false) 
				);
			break;
		case IDC_INST_XANTHA:
      player->EnableInstrument(
				INSTRUMENT_SOUND_XANTHA, 
				(BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_INST_XANTHA) ? true : false) 
				);
			break;

		case IDC_CURRINST_OMMNIBOX:
			EnableWindow(GetDlgItem(hDlg,ActiveInstControls[SelectedInstrument]), true);
			SelectedInstrument = INSTRUMENT_SOUND_BANDFILL;
			EnableWindow(GetDlgItem(hDlg,ActiveInstControls[SelectedInstrument]), false);
			CheckDlgButton(hDlg,ActiveInstControls[SelectedInstrument],BST_CHECKED);
      player->EnableInstrument(
				INSTRUMENT_SOUND_BANDFILL, 
				(BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_INST_OMMNIBOX) ? true : false) 
				);
			break;			
		case IDC_CURRINST_TRAZ:
			EnableWindow(GetDlgItem(hDlg,ActiveInstControls[SelectedInstrument]), true);
			SelectedInstrument = INSTRUMENT_SOUND_TRAZ;
			EnableWindow(GetDlgItem(hDlg,ActiveInstControls[SelectedInstrument]), false);
			CheckDlgButton(hDlg,ActiveInstControls[SelectedInstrument],BST_CHECKED);
      player->EnableInstrument(
				INSTRUMENT_SOUND_TRAZ, 
				(BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_INST_TRAZ) ? true : false) 
				);
			break;			
		case IDC_CURRINST_MANDOVIOL:
			EnableWindow(GetDlgItem(hDlg,ActiveInstControls[SelectedInstrument]), true);
			SelectedInstrument = INSTRUMENT_SOUND_MANDOVIOL;
			EnableWindow(GetDlgItem(hDlg,ActiveInstControls[SelectedInstrument]), false);
			CheckDlgButton(hDlg,ActiveInstControls[SelectedInstrument],BST_CHECKED);
      player->EnableInstrument(
				INSTRUMENT_SOUND_MANDOVIOL, 
				(BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_INST_MANDOVIOL) ? true : false) 
				);
			break;			
		case IDC_CURRINST_CHIDINKALU:
			EnableWindow(GetDlgItem(hDlg,ActiveInstControls[SelectedInstrument]), true);
			SelectedInstrument = INSTRUMENT_SOUND_CHIDINKALU;
			EnableWindow(GetDlgItem(hDlg,ActiveInstControls[SelectedInstrument]), false);
			CheckDlgButton(hDlg,ActiveInstControls[SelectedInstrument],BST_CHECKED);
      player->EnableInstrument(
				INSTRUMENT_SOUND_CHIDINKALU, 
				(BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_INST_CHIDINKALU) ? true : false) 
				);
			break;			
		case IDC_CURRINST_NALARGON:
			EnableWindow(GetDlgItem(hDlg,ActiveInstControls[SelectedInstrument]), true);
			SelectedInstrument = INSTRUMENT_SOUND_NALARGON;
			EnableWindow(GetDlgItem(hDlg,ActiveInstControls[SelectedInstrument]), false);
			CheckDlgButton(hDlg,ActiveInstControls[SelectedInstrument],BST_CHECKED);
      player->EnableInstrument(
				INSTRUMENT_SOUND_NALARGON, 
				(BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_INST_NALARGON) ? true : false) 
				);
			break;			
		case IDC_CURRINST_XANTHA:
			EnableWindow(GetDlgItem(hDlg,ActiveInstControls[SelectedInstrument]), true);
			SelectedInstrument = INSTRUMENT_SOUND_XANTHA;
			EnableWindow(GetDlgItem(hDlg,ActiveInstControls[SelectedInstrument]), false);
			CheckDlgButton(hDlg,ActiveInstControls[SelectedInstrument],BST_CHECKED);
      player->EnableInstrument(
				INSTRUMENT_SOUND_XANTHA, 
				(BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_INST_XANTHA) ? true : false) 
				);
			break;

    case IDC_RANDOMFLOURISH:
      player->EnableRandomFlourish(
        (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_RANDOMFLOURISH) ? true : false) 
        );
      break;

		}
		break;
  case SWGC_ADD_SONG:
    AddSong(static_cast<SongSoundType>(lParam));
    EnumerateSongList(hDlg);
    break;
  case SWGC_STATUS:
    {
      HFONT hfnt, hOldFont; 
      HDC hdc;
      static char textstat[NUM_INSTRUMENTS][50];
      SoundListType * buff = (SoundListType *)lParam;

      for( int y = 0; y < NUM_INSTRUMENTS; y++ )
      {
        textstat[y][0] = '\0';
        for( int bidx = 0; bidx < 2; bidx++ )
        {
          switch( buff[bidx].SoundList[y] )
          {
          case FLOURISH_SOUND_SILENCE:
            strcat_s(textstat[y]," ~rest~");
            break;
          case FLOURISH_SOUND_1:
            strcat_s(textstat[y]," flo[1]");
            break;
          case FLOURISH_SOUND_2:
            strcat_s(textstat[y]," flo[2]");
            break;
          case FLOURISH_SOUND_3:
            strcat_s(textstat[y]," flo[3]");
            break;
          case FLOURISH_SOUND_4:
            strcat_s(textstat[y]," flo[4]");
            break;
          case FLOURISH_SOUND_5:
            strcat_s(textstat[y]," flo[5]");
            break;
          case FLOURISH_SOUND_6:
            strcat_s(textstat[y]," flo[6]");
            break;
          case FLOURISH_SOUND_7:
            strcat_s(textstat[y]," flo[7]");
            break;
          case FLOURISH_SOUND_8:
            strcat_s(textstat[y]," flo[8]");
            break;
          case FLOURISH_SOUND_INTRO:
            strcat_s(textstat[y],"  intro");
            break;
          case FLOURISH_SOUND_IDLE:
            strcat_s(textstat[y],"   idle");
            break;
          case FLOURISH_SOUND_OUTRO:
            strcat_s(textstat[y],"  outro");
            break;
          case FLOURISH_SOUND_START:
            strcat_s(textstat[y],"       ");
            break;
          case FLOURISH_SOUND_STOP:
            strcat_s(textstat[y],"       ");
            break;
          case FLOURISH_SOUND_MAX:
          default:
            strcat_s(textstat[y],"       ");
            break;
          }

          if( bidx == 0 )
          {
            strcat_s(textstat[y]," <--");
          }
        }

        hdc = GetDC( GetDlgItem(hDlg,InstrumentStatusControls[y]) );
        hfnt = (HFONT)GetStockObject(ANSI_FIXED_FONT); 

        if (hOldFont = (HFONT)SelectObject(hdc, hfnt)) 
        { 
          SendMessage( GetDlgItem(hDlg,InstrumentStatusControls[y]), WM_SETFONT, (WPARAM) hfnt, (LPARAM) true );
          SendMessage( GetDlgItem(hDlg,InstrumentStatusControls[y]), WM_SETTEXT, (WPARAM) 0, (LPARAM) textstat[y] );
        }
        SelectObject(hdc, hOldFont); 
        //HeapFree(GetProcessHeap(),0,(void *)lParam);
      }

      retval = true;
    }
    break;
	}
	return retval;
}
