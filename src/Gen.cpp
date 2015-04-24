#include "Gen.h"
#include <boost/foreach.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/path.hpp>

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

using namespace std;
using boost::range::find;
using boost::range::push_back;
using boost::algorithm::replace_first;
namespace fs = boost::filesystem;

bool Gen::isBasicType(const string & type) const
{
	return type == "bool"
	       || type == "int"
	       || type == "double"
	       || type == "char"
	       || type == "float"
	       || type == "long";
}

bool Gen::hasString(const Type & t) const
{
	BOOST_FOREACH(const Field & f, t.fields) {
		if (f.type.find("string") != string::npos) {
			return true;
		}
	}
	BOOST_FOREACH(const Type & st, t.subTypes) {
		if (hasString(st)) {
			return true;
		}
	}
	return false;
}

bool Gen::hasVector(const Type & t) const
{
	BOOST_FOREACH(const Field & f, t.fields) {
		if (f.size < 0) {
			return true;
		}
	}
	BOOST_FOREACH(const Type & st, t.subTypes) {
		if (hasVector(st)) {
			return true;
		}
	}
	return false;
}

bool Gen::hasOptional(const Type & t) const
{
	BOOST_FOREACH(const Field & f, t.fields) {
		if (f.isOptionnal) {
			return true;
		}
	}
	BOOST_FOREACH(const Type & st, t.subTypes) {
		if (hasOptional(st)) {
			return true;
		}
	}
	return false;
}

bool Gen::hasVirtual(const Type & t) const
{
	BOOST_FOREACH(const Field & f, t.fields) {
		if (f.pType && !f.pType->pChildTypes.empty()) {
			return true;
		}
	}
	BOOST_FOREACH(const Type & st, t.subTypes) {
		if (hasVirtual(st)) {
			return true;
		}
	}
	return false;
}

bool Gen::isSubType(const string & n, const Type & t) const
{
	BOOST_FOREACH(const Type & st, t.subTypes) {
		if (n.find(st.name) != string::npos) {
			return true;
		}
		if (isSubType(n, st)) {
			return true;
		}
	}
	return false;
}

string Gen::cppType(const Type & t) const
{
	string ct = t.fullPath;
	boost::replace_all(ct, ".", "::");
	return ct;
}

string Gen::cppType(const Field & f, bool absolute) const
{
	string t = f.type;
	if (absolute && f.pType) {
		t = cppType(*f.pType);
	}
	if (f.type == "string") {
		t = "" + t;
	}
	if (f.pType && !f.pType->pChildTypes.empty()) {
		t = "boost::shared_ptr< " + t + " >";
	}
	if (f.isOptionnal) {
		t = "boost::optional< " + t + " >";
	} else if (f.size < 0) {
		t = "vector< " + t + " >";
	} else if (f.size > 1) {
		t = "boost::array<" + t + "," + boost::lexical_cast<string>(f.size) + ">";
	}
	return t;
}

void Gen::getDepends(const vector<Type> & types, set<string> & depends) const
{
	BOOST_FOREACH(const Type & t, types) {
		for (size_t i = 0; i < t.pChildTypes.size(); i++) {
			depends.insert("\"" + t.pChildTypes[i]->pInterRep->name + ".h\"");
		}
		getDepends(t.subTypes, depends);
	}
}

void Gen::printType(const Type & t, ofstream & out, string indent) const
{

	BOOST_FOREACH(Type * ct, t.pChildTypes) {
		const string ctType = cppType(*ct);
		out << indent << "struct " << ctType << ";" << endl;
		BOOST_FOREACH(Type * cct, ct->pChildTypes) {
			const string cctType = cppType(*cct);
			out << indent << "struct " << cctType << ";" << endl;
		}
	}
	out << endl;
	out << indent << "struct " << t.name << (t.superType.empty() ? "" : " : public " + t.superType ) << " {" << endl;
	out << endl;
	BOOST_FOREACH(const Type & st, t.subTypes) {
		printType(st, out, "\t");
	}

	size_t maxTypeLen = 0;
	BOOST_FOREACH(const Field & f, t.fields) {
		maxTypeLen = max(maxTypeLen, cppType(f).length());
	}

	BOOST_FOREACH(const Field & f, t.fields) {
		const string fType = cppType(f);
		out << indent << "\t" << fType << string(maxTypeLen - fType.length(), ' ') << " " << f.name << ";" << endl;
	}
	out << endl;

	if (!t.pChildTypes.empty()) {
		out << indent << "\tvirtual ~" << t.name << "(){}" << endl;
		out << endl;
	}
	BOOST_FOREACH(Type * ct, t.pChildTypes) {
		const string ctType = cppType(*ct);
		out << indent << "\t" << ctType << " * as_" << ct->name << "();" << endl;
		out << indent << "\tconst " << ctType << " * as_" << ct->name << "() const;" << endl;
		BOOST_FOREACH(Type * cct, ct->pChildTypes) {
			const string cctType = cppType(*cct);
			out << indent << "\t" << cctType << " * as_" << cct->name << "();" << endl;
			out << indent << "\tconst " << cctType << " * as_" << cct->name << "() const;" << endl;
		}
	}
	out << indent << "};" << endl;
}

void Gen::printTypeParse(const Type & t, std::ofstream & out, std::string indent) const
{
	out << "void parse(const boost::property_tree::ptree &, " << t.fullPath << " &);" << endl;
	out << "void put(const " << t.fullPath << " &, boost::property_tree::ptree &, const std::string & path=std::string(\""
	    << t.name << "\"));" << endl;
	BOOST_FOREACH(const Type & st, t.subTypes) {
		printTypeParse(st, out);
	}
}

void Gen::printTypeSrc(const Type & t, ofstream & out, const string & ns) const
{
	BOOST_FOREACH(const Type & st, t.subTypes) {
		printTypeSrc(st, out, ns + t.name + "::");
	}
	out << endl;
	out << "void parse(const ptree & _pt, " << t.fullPath << " & _type) {" << endl;
	if (t.pSuperType) {
		out << "\tparse(_pt, *((" << t.pSuperType->fullPath << "*)&_type));" << endl;
	}
	vector<Field const *> arrays, vectors;
	BOOST_FOREACH(const Field & f, t.fields) {
		if (f.isOptionnal) {
			const string path = (f.isAttr) ? "<xmlattr>." : "";
			out << "\tparse( _pt.get_child_optional(\"" << path << f.nodeName << "\") , _type." << f.name << ");" << endl;
		} else if (f.isAttr) {
			out << "\tparse( _pt.get_child(\"<xmlattr>." << f.nodeName << "\") , _type." << f.name << ");" << endl;
		} else if (f.size < 0) {
			vectors.push_back(&f);
		} else if (f.size > 1) {
			arrays.push_back(&f);
		} else {
			out << "\tparse( _pt.get_child(\"" << f.nodeName << "\") , _type." << f.name << ");" << endl;
		}
	}
	if (!arrays.empty() || !vectors.empty()) {
		BOOST_FOREACH(const Field * f, arrays) {
			out << "\tsize_t " << f->name << "_index = 0;" << endl;
		}
		out << "\tBOOST_FOREACH(ptree::value_type const & _val, _pt.get_child(\"\")) {" << endl;
		BOOST_FOREACH(const Field * f, arrays) {
			const string baseT = f->type;
			out << "\t\tif (_val.first == \"" << f->nodeName << "\") {" << endl;
			out << "\t\t\tparse(_val.second , _type." << f->name << "[" << f->name << "_index]);" << endl;
			out << "\t\t\t" << f->name << "_index++;" << endl;
			out << "\t\t\tcontinue;" << endl;
			out << "\t\t}" << endl;
		}
		BOOST_FOREACH(const Field * f, vectors) {
			out << "\t\tif (_val.first == \"" << f->nodeName << "\") {" << endl;
			out << "\t\t\t" << cppType(*f, true) << "::value_type _tmp;" << endl;
			out << "\t\t\tparse(_val.second , _tmp);" << endl;
			out << "\t\t\t_type." << f->name << ".push_back(_tmp);" << endl;
			out << "\t\t\tcontinue;" << endl;
			out << "\t\t}" << endl;
		}
		out << "\t}" << endl;
	}
	out << "}" << endl;
	out << endl;

	out << "void put(const " << t.fullPath << " & _type, ptree & _pt, const string & _path) {" << endl;
	out << "\tconst string _prefix = _path.empty() ? \"\" : _path+\".\";" << endl;
	BOOST_FOREACH(const Field & f, t.fields) {
		if (f.isOptionnal) {
			const string path = (f.isAttr) ? "<xmlattr>." : "";
			out << "\tput(  _type." << f.name << ", _pt, _prefix+\"" << path << f.nodeName << "\" );" << endl;
		} else if (f.isAttr) {
			out << "\tput(  _type." << f.name << ", _pt, _prefix+\"<xmlattr>."  << f.nodeName << "\" );" << endl;
		} else if (f.size < 0) {
			out << "\tBOOST_FOREACH(const " << cppType(f, true) << "::value_type & _tmp, _type." << f.name << ") {" << endl;
			out << "\t\tptree & _rPt = _pt.add(_prefix+\"" << f.nodeName << "\", \"\");" << endl;
			out << "\t\tput(_tmp, _rPt, \"\");" << endl;
			out << "\t}" << endl;
		} else if (f.size > 1) {
			out << "\tfor (size_t _i = 0; _i < " << f.size << "; _i++) {" << endl;
			out << "\t\tput(_type." << f.name << "[_i], _pt, _prefix+\"" << f.nodeName << "\" );" << endl;
			out << "\t}" << endl;
		} else {
			out << "\tput(  _type." << f.name << ", _pt, _prefix+\""  << f.nodeName << "\" );" << endl;
		}
	}
	if (!t.pChildTypes.empty()) {
		BOOST_FOREACH(Type * vt, t.pChildTypes) {
			BOOST_FOREACH(Type * cvt, vt->pChildTypes) {
				const string cvtType = cppType(*cvt);
				const string cvarName = "sub_" + cvt->name;
				out << "\tconst " << cvtType << " * " << cvarName << " = _type.as_" << cvt->name << "();" << endl;
				out << "\tif (" << cvarName << ") {" << endl;
				out << "\t\tput( \"" << cvt->name << "\", _pt, _prefix+\"<xmlattr>.xsi:type\");" << endl;
				out << "\t\tput(*" << cvarName << ", _pt, _path);" << endl;
				out << "\t\treturn;" << endl;
				out << "\t}" << endl;
			}
			const string vtType = cppType(*vt);
			const string varName = "sub_" + vt->name;
			out << "\tconst " << vtType << " * " << varName << " = _type.as_" << vt->name << "();" << endl;
			out << "\tif (" << varName << ") {" << endl;
			out << "\t\tput( \"" << vt->name << "\", _pt, _prefix+\"<xmlattr>.xsi:type\");" << endl;
			out << "\t\tput(*" << varName << ", _pt, _path);" << endl;
			out << "\t\treturn;" << endl;
			out << "\t}" << endl;
		}
	}
	out << "}" << endl;

	if (!t.pChildTypes.empty()) {
		out << "void parse(const ptree & _pt, boost::shared_ptr<" << t.name << "> & _t) {" << endl;
		out << "\tconst string _baseType = _pt.get<string>(\"<xmlattr>.xsi:type\", \"\");" << endl;
		BOOST_FOREACH(Type * vt, t.pChildTypes) {
			out << "\tif (_baseType == \"" << vt->name << "\") {" << endl;
			const string vtType = cppType(*vt);
			out << "\t\t_t.reset((" << t.name << "*)new " << vtType << "());" << endl;
			//out << "\t\tparse(_pt, *_t.get());" << endl;
			out << "\t\tparse(_pt, (" << vtType << "&)*_t.get());" << endl;
			out << "\t\treturn;" << endl;
			out << "\t}" << endl;
		}
		out << "}" << endl;
	}
}

void Gen::genHeader(const string & outDir, const string & fileName, const InterRep & rep, const string & ns) const
{
	const string filePath = outDir + "/" + fileName + ".h";
	cout << "Writing file : " << filePath << endl;
	ofstream out(filePath.c_str());
	if (out.bad()) {
		throw runtime_error("Cannot write to : " + filePath);
	}
	out << "#pragma once" << endl;
	BOOST_FOREACH(const string & dep, rep.dependencies) {
		out << "#include \"" << fs::path(dep).stem().string() << ".h\"" << endl;
	}
	set<string> depends;
	depends.insert("<string>");
	depends.insert("<boost/array.hpp>");
	BOOST_FOREACH(const Type & t, rep.types) {
		//if (hasString(t)) {
		//	depends.insert("<string>");
		//}
		if (hasVector(t)) {
			depends.insert("<vector>");
		}
		if (hasVirtual(t)) {
			depends.insert("<boost/shared_ptr.hpp>");
		}
		if (hasOptional(t)) {
			depends.insert("<boost/optional.hpp>");
		}
	}
	BOOST_FOREACH(const string & dep, depends) {
		out << "#include " << dep << endl;
	}
	out << "#include <functional>" << endl;
	out << endl;
	out << "namespace " << ns << " {" << endl;
	out << "using namespace std;" << endl;
	out << endl;

	BOOST_FOREACH(const Enum & e, rep.enums) {
		out << "enum " << e.name << " {" << endl;
		string prefix = e.name + "_";
		boost::to_upper(prefix);
		BOOST_FOREACH(const string & v, e.values) {
			out << "\t" << prefix << v << "," << endl;
		}
		out << "};" << endl;
	}
	out << endl;
	BOOST_FOREACH(const Type & t, rep.types) {
		printType(t, out);
	}
	out << endl;
	BOOST_FOREACH(const Enum & e, rep.enums) {
		out << e.name << " str2" << e.name << "(const string & str);" << endl;
		out << "string " << e.name << "2str(const " << e.name << " &);" << endl;
	}
	out << "}" << endl;
	out << "namespace boost { namespace property_tree { template<class Key,class Data,class KeyCompare>class basic_ptree; typedef basic_ptree<std::string,std::string,std::less<std::string> > ptree; } }"
	    << endl;
	out << "namespace " << ns << " {" << endl;
	out << "namespace parser {" << endl;
	BOOST_FOREACH(const Type & t, rep.types) {
		printTypeParse(t, out);
	}
	out << "}" << endl;
	out << "}" << endl;
}

void Gen::genSource(const string & outDir, const string & fileName, const InterRep & rep, const string & ns) const
{
	const string filePath = outDir + "/" + fileName + ".cpp";
	cout << "Writing file : " << filePath << endl;
	ofstream out(filePath.c_str());
	if (out.bad()) {
		throw runtime_error("Cannot write to : " + filePath);
	}
	out << "#include \"" << fileName << ".h\"" << endl;
	out << "#include <boost/property_tree/ptree.hpp>" << endl;
	out << "#include <boost/foreach.hpp>" << endl;
	out << "#include <boost/optional.hpp>" << endl;
	out << "#include <boost/shared_ptr.hpp>" << endl;
	set<string> depends;
	getDepends(rep.types, depends);
	BOOST_FOREACH(const string & dep, depends) {
		out << "#include " << dep << endl;
	}
	out << endl;
	out << "using namespace std;" << endl;
	out << "using boost::property_tree::ptree;" << endl;
	out << "namespace " << ns << " {" << endl;
	set<Type *> virtualTypes;
	BOOST_FOREACH(const Type & t, rep.types) {
		const string tType = cppType(t);
		BOOST_FOREACH(Type * ct, t.pChildTypes) {
			virtualTypes.insert(ct);
			BOOST_FOREACH(Type * cct, ct->pChildTypes) {
				virtualTypes.insert(cct);
				const string cctType = cppType(*cct);
				out << cctType << " * " << tType << "::as_" << cct->name << "() { return dynamic_cast<" << cctType << "*>(this); }" <<
				    endl;
				out << "const " << cctType << " * " << tType << "::as_" << cct->name << "() const { return dynamic_cast<const " <<
				    cctType << "*>(this); }" << endl;
			}
			const string ctType = cppType(*ct);
			out << ctType << " * " << tType << "::as_" << ct->name << "() { return dynamic_cast<" << ctType << "*>(this); }" <<
			    endl;
			out << "const " << ctType << " * " << tType << "::as_" << ct->name << "() const { return dynamic_cast<const " << ctType
			    << "*>(this); }" << endl;
		}
	}
	if (!rep.enums.empty()) {
		out << "template<size_t N> size_t findEnumVal(const string(& values)[N], const string & str) { size_t t = distance(values, find(values, values + N, str));	if (t >= N) throw domain_error(\"Invalid Enum Val \" + str); return t; }"
		    << endl;
	}
	BOOST_FOREACH(const Enum & e, rep.enums) {
		out << "static const string " << e.name << "strVals[] = {" << endl;
		BOOST_FOREACH(const string & val, e.values) {
			out << "\t\"" << val << "\"," << endl;
		}
		out << "};" << endl;
		out << e.name << " str2" << e.name << "(const string & str) {" << endl;
		out << "\treturn (" << e.name << ")findEnumVal(" << e.name << "strVals, str);" << endl;
		out << "}" << endl;
		out << "string " << e.name << "2str(const " << e.name << " & v) {" << endl;
		out << "\treturn " << e.name << "strVals[(unsigned int)v];" << endl;
		out << "}" << endl;
	}
	out << "namespace parser {" << endl;
	out << "template<typename T>" << endl;
	out << "void parse(const ptree & pt, T & t) {" << endl;
	out << "\tt = pt.get_value<T>();" << endl;
	out << "}" << endl;
	out << "template<typename T>" << endl;
	out << "void put(const T & t, ptree & pt, const string & path) {" << endl;
	out << "\tpt.put(path, t);" << endl;
	out << "}" << endl;
	BOOST_FOREACH(const Enum & e, rep.enums) {
		out << "void parse(const ptree & pt, " << e.name << " & e) {" << endl;
		out << "\te = str2" << e.name << "(pt.get_value<string>());" << endl;
		out << "}" << endl;
		out << "void put(const " << e.name << " & e, ptree & pt, const string & path) {" << endl;
		out << "\tpt.put(path, " << e.name << "2str(e));" << endl;
		out << "}" << endl;
	}
	out << "template<typename T> void parse(boost::optional<const ptree &> pt, boost::optional<T> & t) {	if (pt) { T tmp; parse(pt.get(), tmp); t = tmp; } }"
	    << endl;
	out << "template<typename T> void parse(const ptree & pt, boost::shared_ptr<T> & t) { T * tmp = new T(); parse(pt, *tmp); t.reset(tmp); }"
	    << endl;
	out << "template<typename T> void put(const boost::optional<T> & t, ptree & pt, const string & path) { if (t) { put(t.get(), pt, path); } }"
	    << endl;
	out << "template<typename T> void put(const boost::shared_ptr<T> & t, ptree & pt, const string & path) { if (t.get()) { put(*t.get(), pt, path); } }"
	    << endl;
	BOOST_FOREACH(const Type & t, rep.types) {
		printTypeSrc(t, out);
	}
	out << endl;
	out << "}" << endl;
	out << "}" << endl;
}
