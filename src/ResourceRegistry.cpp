#include "stdafx.h"
#include "ResourceRegistry.h"
#include "Debug.h"
#include "TREFile.h"
#include "TOCFile.h"


ResourceRegistry::ResourceRegistry()
{
  this->resources = new std::vector<TREResource *>;
  this->musicResources = new std::vector<TREResource *>;
}

bool ResourceRegistry::findByTRE(std::string directory)
{
  bool success = true;

  WIN32_FIND_DATA findFileData;
  HANDLE hFind;

  std::string glob = directory + "/*.tre";

  hFind = FindFirstFile(glob.c_str(), &findFileData);
  if (hFind == INVALID_HANDLE_VALUE)
  {
    TRACE("FindFirstFile failed (%d)\n", GetLastError());
    success = false;
  }
  else
  {
    BOOL continuing = TRUE;
    do
    {
      TRACE("Reading [%s]", findFileData.cFileName);
      TREFile *treFile = new TREFile(findFileData.cFileName);
      std::vector<TREResource *> *treResources = treFile->load("player_music/sample/");
      this->resources->insert(this->resources->end(), treResources->begin(), treResources->end());
      continuing = FindNextFile(hFind, &findFileData);
    } while (continuing);
    FindClose(hFind);

    for (std::vector<TREResource *>::iterator it = this->resources->begin(); it != this->resources->end(); ++it)
    {
        this->musicResources->push_back(*it);
        int songNum = -1;
        if (sscanf_s((*it)->name, "player_music/sample/song%02d", &songNum) == 1)
        {
          TRACE("Inserted song: %02d", songNum);
          this->songs.insert(songNum);
        }
    }

  }
  return success;
}

bool ResourceRegistry::findByTOC(std::string directory)
{
  bool success = true;

  WIN32_FIND_DATA findFileData;
  HANDLE hFind;

  std::string glob = directory + "/*.toc";

  hFind = FindFirstFile(glob.c_str(), &findFileData);
  if (hFind == INVALID_HANDLE_VALUE)
  {
    TRACE("FindFirstFile failed (%d)\n", GetLastError());
    success = false;
  }
  else
  {
    BOOL continuing = TRUE;
    do
    {
      TRACE("Reading [%s]", findFileData.cFileName);
      TOCFile *tocFile = new TOCFile(findFileData.cFileName);
      std::vector<TREResource *> *treResources = tocFile->load("player_music/sample/");
      this->resources->insert(this->resources->end(), treResources->begin(), treResources->end());
      continuing = FindNextFile(hFind, &findFileData);
    } while (continuing);
    FindClose(hFind);

    for (std::vector<TREResource *>::iterator it = this->resources->begin(); it != this->resources->end(); ++it)
    {
        this->musicResources->push_back(*it);
        int songNum = -1;
        if (sscanf_s((*it)->name, "player_music/sample/song%02d", &songNum) == 1)
        {
          TRACE("Inserted song: %02d", songNum);
          this->songs.insert(songNum);
        }
    }

  }
  return success;
}

bool ResourceRegistry::find(std::string directory)
{
  bool success = true;

  success = success && this->findByTRE(directory);
  success = success && this->findByTOC(directory);

  return true;
}

TREResource * ResourceRegistry::resource(std::string key)
{
  TREResource *result = NULL;

  for (std::vector<TREResource *>::iterator it = this->musicResources->begin(); it != this->musicResources->end(); ++it)
  {
    if (strcmp((*it)->name, key.c_str()) == 0)
    {
      result = (*it);
      break;
    }
  }

  return result;
}
