#include "gen/includes/Test.h"
#include "gen/includes/TestIncl.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <iostream>

using namespace std;
using namespace boost::property_tree;

void printTree(const ptree & pt, const string & indent = string())
{
	ptree::const_iterator end = pt.end();
	for (ptree::const_iterator it = pt.begin(); it != end; ++it) {
		cout << indent << it->first << " : {" << it->second.get_value<string>() << "}" << endl;
		printTree(it->second, indent + "\t");
	}
}

void printTest(const tst::testDefinition & test)
{
	cout << "Test : " << endl;
	cout << "   strAtt = " << test.strAttr << endl;
	cout << "   longAttr = " << test.longAttr << endl;
	cout << "   dblAtt = " << test.dblAttr << endl;
	cout << "   boolAtt = " << test.boolAttr << endl;
	cout << "   enumAttr = " << tst::testEnum2str(test.enumAttr) << endl;
	if (test.strAttrOpt) {
		cout << "   strAtt = " << test.strAttrOpt << endl;
	}
}


ptree loadTest(tst::testDefinition & test, const string & filePath)
{
	ptree pt;
	ifstream tstStr(filePath.c_str());
	if (!tstStr) {
		cerr << "no file " << filePath << endl;
		return pt;
	}
	xml_parser::read_xml(tstStr, pt);

	try {
		tst::parser::parse(pt.get_child("test"), test);
	} catch (const std::runtime_error & e) {
		cerr << e.what() << endl;
		return ptree();
	}
	return pt;
}

ptree writeTest(const tst::testDefinition & t, const string & filePath)
{
	ptree pt;
	tst::parser::put(t, pt, "test");
	xml_parser::xml_writer_settings<ptree::key_type> settings('\t', 1);
	xml_parser::write_xml(filePath, pt, std::locale(), settings);
	return pt;
}

int main(int , char **)
{
	try {
		tst::testDefinition test;
		ptree ptTest = loadTest(test, "test.xml");
		printTest(test);
	} catch (const std::exception & e) {
		cerr << e.what() << endl;
	}
	return 0;
}

