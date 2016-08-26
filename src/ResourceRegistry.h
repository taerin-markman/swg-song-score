#pragma once
#include "stdafx.h"
#include <vector>
#include <set>
#include <string>
#include "TREResource.h"

class ResourceRegistry
{
protected:
  std::vector<TREResource *> *resources;
  std::vector<TREResource *> *musicResources;
  BOOL findByTRE(std::string directory);
  BOOL findByTOC(std::string directory);

public:
  ResourceRegistry();
  BOOL find(std::string directory);
  TREResource * resource(std::string key);
  std::set<int> songs;
};
