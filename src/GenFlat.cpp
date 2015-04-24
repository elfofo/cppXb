#include "GenFlat.h"
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


string GenFlat::cppType(const Field & f, bool absolute) const
{
	string t = f.type;
	if (absolute && f.pType) {
		t = Gen::cppType(*f.pType);
	}
	if (f.type == "string") {
		t = "" + t;
	}
	if (f.isOptionnal) {
		t = "boost::optional< " + t + " >";
	} else if (f.size < 0) {
		t = "vector< " + t + " >";
	} else if (f.size > 1) {
		t = t + "[" + boost::lexical_cast<string>(f.size) + "]";
	}
	return t;
}

void GenFlat::printType(const Type & t, ofstream & out, string indent) const
{
	out << indent << "struct " << t.name << (t.superType.empty() ? "" : " : public " + t.superType ) << " {" << endl;

	BOOST_FOREACH(const Type & st, t.subTypes) {
		GenFlat::printType(st, out, "\t");
	}

	size_t maxTypeLen = 0;
	BOOST_FOREACH(const Field & f, t.fields) {
		maxTypeLen = max(maxTypeLen, cppType(f).length());
	}

	BOOST_FOREACH(const Field & f, t.fields) {
		const string fType = cppType(f);
		out << indent << "\t" << fType << string(maxTypeLen - fType.length(), ' ') << " " << f.name << ";" << endl;
	}

	out << indent << "};" << endl;
	out << indent << endl;
}

void GenFlat::printTypeSrc(const Type & t, ofstream & out, const std::string & ns) const
{
	BOOST_FOREACH(const Type & st, t.subTypes) {
		GenFlat::printTypeSrc(st, out, ns + t.name + "::");
	}
	out << "void parse(const ptree & _pt, " << t.fullPath << " & _type) {" << endl;
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

	out << "void put(const " << t.fullPath << " & _type, ptree & _pt) {" << endl;
	out << "}" << endl;
}

void GenFlat::genHeader(const std::string & outDir, const string & fileName, const InterRep & rep,
                        const string & ns) const
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
	BOOST_FOREACH(const Type & t, rep.types) {
		if (hasString(t)) {
			depends.insert("<string>");
		}
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

void GenFlat::genSource(const std::string & outDir, const string & fileName, const InterRep & rep,
                        const string & ns) const
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
	out << endl;
	out << "using namespace std;" << endl;
	out << "using boost::property_tree::ptree;" << endl;
	out << "namespace " << ns << " {" << endl;
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
	out << "\t t = pt.get_value<T>();" << endl;
	out << "}" << endl;
	BOOST_FOREACH(const Enum & e, rep.enums) {
		out << "void parse(const ptree & pt, " << e.name << " & e) {" << endl;
		out << "\te = str2" << e.name << "(pt.get_value<string>());" << endl;
		out << "}" << endl;
	}
	out << "template<typename T> void parse(boost::optional<const ptree &> pt, boost::optional<T> & t) {	if (pt) { T tmp; parse(pt.get(), tmp); t = tmp; } }"
	    << endl;
	BOOST_FOREACH(const Type & t, rep.types) {
		printTypeSrc(t, out);
	}
	out << endl;
	out << "}" << endl;
	out << "}" << endl;
}