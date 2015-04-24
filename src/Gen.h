#pragma once
#include "InterRep.h"
#include <string>
#include <vector>


class Gen {
public:
	virtual void genHeader(const std::string & outDir,
	                       const std::string & fileName,
	                       const InterRep & rep,
	                       const std::string & ns) const;

	virtual void genSource(const std::string & outDir,
	                       const std::string & fileName,
	                       const InterRep & rep,
	                       const std::string & ns) const;
protected:
	virtual bool isBasicType(const std::string & type) const;
	virtual bool hasString(const Type & t) const;
	virtual bool hasVector(const Type & t) const;
	virtual bool hasOptional(const Type & t) const;
	virtual bool hasVirtual(const Type & t) const;
	virtual bool isSubType(const std::string & n, const Type & t) const;
	virtual std::string cppType(const Type & t) const;
	virtual std::string cppType(const Field & f, bool absolute = false) const;
	virtual void getDepends(const std::vector<Type> & types, std::set<std::string> & depends) const;
	virtual void printType(const Type & t, std::ofstream & out, std::string indent = std::string()) const;
	virtual void printTypeSrc(const Type & t, std::ofstream & out, const std::string & ns = std::string()) const;
	virtual void printTypeParse(const Type & t, std::ofstream & out, std::string indent = std::string()) const;
};
