#pragma once
#include "Gen.h"
#include "InterRep.h"
#include <string>
#include <vector>

class GenFlat : public Gen {
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

	virtual std::string cppType(const Field & f, bool absolute = false) const;

	virtual void printType(const Type & t, std::ofstream & out, std::string indent = std::string()) const;
	virtual void printTypeSrc(const Type & t, std::ofstream & out, const std::string & ns = std::string()) const;
};