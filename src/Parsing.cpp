#include "Parsing.h"
#include <map>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
namespace bpt = boost::property_tree;
namespace fs = boost::filesystem;
using bpt::ptree;

string parseType(const string & type)
{
	if (type.find("xs:") == 0) {
		const string baseType = type.substr(3);
		if (baseType == "boolean") {
			return "bool";
		}
		if (baseType == "integer") {
			return "int";
		}
		if (baseType == "byte") {
			return "char";
		}
		return baseType;
	}
	return type;
}

Field parseAttribute(const ptree & a)
{
	Field f;
	f.nodeName = a.get<string>("<xmlattr>.name");
	f.name = f.nodeName;
	f.type = parseType(a.get<string>("<xmlattr>.type"));
	if (f.name == f.type) {
		f.name = f.name + "_val";
	}
	f.isAttr = true;
	f.size = 1;
	const string opt = a.get<string>("<xmlattr>.use", "optional");
	f.isOptionnal = (opt == "optional");
	if (f.isOptionnal) {
		f.defVal = a.get<string>("<xmlattr>.default", "");
	}
	return f;
}

Field parseElement(const ptree & a, const string & type = string())
{
	const string maxOccurs = a.get<string>("<xmlattr>.maxOccurs", "1");
	const string minOccurs = a.get<string>("<xmlattr>.minOccurs", "1");
	Field f;
	f.nodeName = a.get<string>("<xmlattr>.name");
	f.name = f.nodeName;
	f.type = parseType(a.get<string>("<xmlattr>.type", type));
	if (f.type.empty()) {
		throw runtime_error("element " + f.name + " has no type");
	}
	if (f.name == f.type) {
		f.name = f.name + "_val";
	}
	f.isAttr = false;
	f.isOptionnal = false;
	if (maxOccurs == "1" && minOccurs == "1") {
		f.size = 1;
	} else if (maxOccurs == minOccurs) {
		f.size = boost::lexical_cast<int>(maxOccurs);
	} else if (maxOccurs == "1" && minOccurs == "0") {
		f.isOptionnal = true;
		f.size = 0;
	} else {
		f.size = -1;
	}
	return f;
}

Type parseType(const ptree & n);
void parseContent(const ptree & node, Type & t)
{
	BOOST_FOREACH(ptree::value_type const & att, node.get_child("")) {
		if (att.first == "xs:attribute") {
			t.fields.push_back(parseAttribute(att.second));
		} else if (att.first == "xs:complexContent") {
			BOOST_FOREACH(ptree::value_type const & ext, att.second.get_child("")) {
				if (ext.first == "xs:extension") {
					t.superType = ext.second.get<string>("<xmlattr>.base");
					parseContent(ext.second, t);
				}
			}
		} else if (att.first == "xs:sequence") {
			BOOST_FOREACH(ptree::value_type const & elt, att.second.get_child("")) {
				if (elt.first == "xs:element") {
					if (elt.second.size() == 1) {
						t.fields.push_back(parseElement(elt.second));
					} else {
						const ptree & nSubT = elt.second.get_child("xs:complexType");
						Type subT;
						subT.name = elt.second.get<string>("<xmlattr>.name") + "_t";
						parseContent(nSubT, subT);
						t.subTypes.push_back(subT);
						t.fields.push_back(parseElement(elt.second, subT.name));
					}
				}
			}
		}
	}
}

Type parseType(const ptree & n)
{
	Type t;
	t.name = n.get<string>("<xmlattr>.name");
	parseContent(n, t);
	return t;
}

Enum parseEnum(const string & name, const ptree & n)
{
	Enum e;
	e.name = name;
	BOOST_FOREACH(ptree::value_type const & val, n.get_child("")) {
		if (val.first == "xs:enumeration") {
			e.values.push_back(val.second.get<string>("<xmlattr>.value"));
		}
	}
	return e;
}

bool dependsOn(const Type & t1, const Type & t2)
{
	if (!t2.superType.empty() && t2.superType == t1.name) {
		return true;
	}
	BOOST_FOREACH(const Field & f2, t2.fields) {
		if (f2.type.find(t1.name) != string::npos) {
			return true;
		}
	}
	BOOST_FOREACH(const Type & st2, t2.subTypes) {
		if (dependsOn(t1, st2)) {
			return true;
		}
	}
	return false;
}

void sortTypes(vector<Type> & types)
{
	BOOST_FOREACH(Type & t, types) {
		sortTypes(t.subTypes);
	}
	if (types.size() > 1) {
		for (size_t k = 0; k < types.size(); k++) {
			for (size_t i = 0; i < types.size() - 1; i++) {
				for (size_t j = i + 1; j < types.size(); j++) {
					if (dependsOn(types[j], types[i])) {
						swap(types[i], types[j]);
					}
				}
			}
		}
	}
}

void printType(const Type & t)
{
	cout << t.name;
	if (!t.subTypes.empty()) {
		cout << "(";
		BOOST_FOREACH(const Type & st, t.subTypes) {
			printType(st);
			cout << ", ";
		}
		cout << ")";
	}
}

InterRep parseFile(const string & filePath)
{
	InterRep rep;
	rep.name = fs::path(filePath).stem().string();
	ptree pt;
	bpt::xml_parser::read_xml(filePath, pt);

	BOOST_FOREACH(ptree::value_type const & n, pt.get_child("xs:schema")) {
		if (n.first == "xs:complexType") {
			rep.types.push_back(parseType(n.second));
		} else if (n.first == "xs:simpleType") {
			const string typeName = n.second.get<string>("<xmlattr>.name");
			rep.enums.push_back(parseEnum(typeName, n.second.get_child("xs:restriction")));
		} else if (n.first == "xs:include") {
			fs::path depPath(n.second.get<string>("<xmlattr>.schemaLocation"));
			string parentPath = depPath.parent_path().string();
			if (!parentPath.empty()) {
				parentPath += "/";
			}
			rep.dependencies.insert(parentPath + depPath.stem().string());
		}
	}

	sortTypes(rep.types);

	return rep;
};

void getAllTypes(vector<Type> & types, vector<Type *> & allTypes)
{
	BOOST_FOREACH(Type & t, types) {
		allTypes.push_back(&t);
		getAllTypes(t.subTypes, allTypes);
	}
}

void link(InterRep * interRep, vector<Type> & types, Type * parentType = 0)
{
	const string prefixPath = (parentType != 0) ? parentType->fullPath + "::" : "";
	BOOST_FOREACH(Type & t, types) {
		t.fullPath = prefixPath + t.name;
		t.pParentType = parentType;
		t.pInterRep = interRep;
		BOOST_FOREACH(Type & st, t.subTypes) {
			t.subTypesByNames[st.name] = &st;
		}
		BOOST_FOREACH(Field & f, t.fields) {
			f.pParentType = &t;
		}
		link(interRep, t.subTypes, &t);
	}
}

void link(vector<InterRep> & reps)
{
	vector<Type *> allTypes;
	map<string, Type *> allBaseTypes;
	map<string, Enum *> allEnums;
	BOOST_FOREACH(InterRep & rep, reps) {
		BOOST_FOREACH(Enum & e, rep.enums) {
			allEnums[e.name] = &e;
			rep.enumsByName[e.name] = &e;
		}
		link(&rep, rep.types);
		getAllTypes(rep.types, allTypes);
		BOOST_FOREACH(Type & t, rep.types) {
			allBaseTypes[t.name] = &t;
			rep.typesByNames[t.name] = &t;
		}
	}
	map<string, vector<Type *> > subTypes;
	BOOST_FOREACH(Type * t, allTypes) {
		if (!t->superType.empty()) {
			subTypes[t->superType].push_back(t);
			if (allBaseTypes.find(t->superType) != allBaseTypes.end()) {
				t->pSuperType = allBaseTypes[t->superType];
			}
		} else {
			t->pSuperType = 0;
		}
	}
	BOOST_FOREACH(Type * t, allTypes) {
		BOOST_FOREACH(Field & f, t->fields) {
			f.pEnum = 0;
			map<string, Type *>::iterator itT = t->subTypesByNames.find(f.type);
			if (itT != t->subTypesByNames.end()) {
				f.pType = itT->second;
			} else {
				itT = t->pInterRep->typesByNames.find(f.type);
				if (itT != t->pInterRep->typesByNames.end()) {
					f.pType = itT->second;
				} else {
					f.pType = 0;
					map<string, Enum *>::iterator itE = t->pInterRep->enumsByName.find(f.type);
					if (itE != t->pInterRep->enumsByName.end()) {
						f.pEnum = itE->second;
					}
				}
			}
		}
		map<string, vector<Type *> >::iterator subs = subTypes.find(t->name);
		if (subs == subTypes.end()) {
			continue;
		}
		t->pChildTypes = subs->second;
	}
	/*BOOST_FOREACH(Type * t, allTypes) {
	    vector<Type *> subChildren;
	    BOOST_FOREACH(Type * childType, t->pChildTypes) {
	        subChildren.insert(subChildren.end(), childType->pChildTypes.begin(), childType->pChildTypes.end());
	    }
	    t->pChildTypes.insert(t->pChildTypes.begin(), subChildren.begin(), subChildren.end());
	}*/

}