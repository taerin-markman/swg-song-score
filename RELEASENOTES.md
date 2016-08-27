# SWG Song Score v0.9.4.0

SWG Song Score is a small utility that emulates a musician from outside the game. It does not interact with the game in any way, but it does require that you have SWG installed. Just unzip it to your SWG installation folder and run it from there. Suggestions are very welcome! (Just keep in mind I'm a noob Win32 programmer. =) )

## New changes in v0.9.4.0:

- There is now a History box that creates a macro suitable for in-game corresponding to what is being played. Just copy and paste its contents. Note that there are some actions the randomizer can take that you cannot take in a macro, such as telling a band instrument other than yours to play a pause (silent) flourish. But it should get close to what you're hearing.

## New changes in v0.9.3.3:

- Introduces a re-write of the .tre/.toc file parsing.

## New changes in v0.9.3.0:

- In anticipation for chapter 5 patch release, this version includes the "auto-discovery" feature I've talked about before. Recognized songs will be named while unrecognized songs (songs made available in SWG after this release of Song Score) will still be found and usable, but just won't have a name. It should recognize #20 as "Boogie" and #13 as "Carnival" when those become available, however, since we've been told about those in advance.

## New changes in v0.9.2.3:

- Calypso! (only this one actually works!)

## New changes in v0.9.2.2:

- Calypso!

## New changes in v0.9.2.1:

- Music now continues playing when SWG Song Score is not the active window.
- You can now set an option to randomize flourishes and let the tool play for you. (This is fun.) It will randomize the flourishes for each active instrument, putting varying degrees of preference on bandflourishes and idle flourishes so it's not a jumbled mess.

## New changes in v0.9.2.0:

- Changed the way files are read so that they don't break with patches (and so the tool works on both test center and live versions).
- Added new song, Swing.
- Note: Future versions (hopefully very soon) will automatically detect the presence of new songs. The tech is present in this version, but I wanted to make this available before working on changing the UI to accommodate dynamic addition of new songs. The next version will also be quite a bit more graceful to errors. This version is sort of a throw-together, temporarily.

## New changes in v0.9.0.4:

- Added new songs, Funk and StarWars4. I hope I have them labelled correctly. Took a guess on which was which. =)
- Fixed a gnarly memory leak. Gnarly on the order of several MB every time a flourish was played (oops).
- Fixed a few crash related bugs.
- Added /changemusic command.

## New changes in v0.9.0.3:

- Added Waltz (oops).
- Removed dependency on Windows XP. I've tested this version on Win98 SE without any remaining problems.
- Added version number next to title.

## New changes in v0.9.0.2:

- Queue up to six flourishes for each instrument individually by selecting the current instrument and pressing the single flourish button. (DEVIATION: Game queues 5 flourishes only and flushes the queue every tick (10 seconds). I'm queuing 6 flourishes and never flush the queue.)
- Added checkboxes to enable or disable instruments. If an instrument is enabled/disabled during playback, the change will take effect on the next flourish. (DEVIATION: This is a deviation from in-game where you could move your avatar and stop playback instantaneously, but I opted for the more graceful approach.)
- Added accellerators (hot keys). Shift + F1 through Shift + F8 will bandflourish. F1 through F8 will single instrument flourish, and F9 will /pausemusic flourish the current instrument. (DEVIATION: /pausemusic in game doesn't appear to get queued up with other flourishes. Not sure exactly what happens to the flourish queue, really, so I currently have it queued along with the rest of the regular flourishes.)
- Removed dependency on MFC71.DLL (rewrote app using straight Win32 API)--this makes it so no installer is needed.

## New changes in v0.9.0.1:

- First version.

ps. Please remember to virus scan everything you download from the internet. I made this on a computer with a virus scanner running, but you can never be sure.
