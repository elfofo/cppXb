#pragma once

#include <string>
#include <vector>
#include "InterRep.h"

InterRep parseFile(const std::string & filePath);

void link(std::vector<InterRep> & reps);

void sortTypes(std::vector<Type> & types);