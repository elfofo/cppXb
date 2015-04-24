#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>

#ifndef foreach
#define foreach(a, b) for(a : b)
#endif

struct Type;
struct InterRep;
struct Enum;

struct Field {

	std::string name;//this is the final field name

	std::string nodeName;//this is the xml node name to be parse

	std::string type;//the type name

	std::string defVal;

	bool isAttr;
	int  size;// [< 0 : dynamic arrays][> 1 : fixed arrays]
	bool isOptionnal;

	Type * pType;
	Type * pParentType;
	Enum * pEnum;
};

struct Enum {

	std::string name;

	std::vector<std::string> values;
};

struct Type {

	std::string name;

	std::string fullPath;

	std::string superType;

	std::vector<Field> fields;

	std::vector<Type> subTypes;
	std::map<std::string, Type *> subTypesByNames;

	Type * pParentType;//for subTypes
	Type * pSuperType;
	std::vector<Type *> pChildTypes; //types derivated from this
	InterRep * pInterRep;
};

struct InterRep {
	std::string name;
	std::set<std::string> dependencies;
	std::vector<Type> types;
	std::vector<Enum> enums;

	std::map<std::string, Type *> typesByNames;
	std::map<std::string, Enum *> enumsByName;
};