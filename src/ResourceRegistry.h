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
  bool findByTRE(std::string directory);
  bool findByTOC(std::string directory);

public:
  ResourceRegistry();
  bool find(std::string directory);
  TREResource * resource(std::string key);
  std::set<int> songs;
};
