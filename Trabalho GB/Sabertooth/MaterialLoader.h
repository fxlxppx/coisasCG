#pragma once
#include <string>
#include <map>
#include "Material.h"

std::map<std::string, Material*> loadMTL(const std::string& path);