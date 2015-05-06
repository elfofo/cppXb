#include "Parsing.h"
#include "Gen.h"
#include "GenFlat.h"
#include <boost/filesystem.hpp>
//#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <iostream>
#include <string>

using namespace std;
namespace fs = boost::filesystem;
string ns = "xsdgen";
string outDirH = "gen/includes";
string outDirC = "gen/src";
bool recursive = false;

int fatal(const string & msg)
{
	cout << msg << endl;
	system("pause");
	return -1;
}

void prepareOutDir(const fs::path & outPath)
{
	if (!fs::exists(outPath)) {
		cout << "Creating out dir : " << outPath.string() << endl;
		if (!fs::create_directories(outPath)) {
			throw runtime_error("Cannot create out dir " + outPath.string());
		}
	}
}

void doParsing(const fs::path & path, vector<InterRep> & reps, vector<fs::path> & paths)
{
	if (path.extension().string() == ".xsd") {
		try {
			cout << "Parsing " << path << endl;
			reps.push_back(parseFile(path.string()));
			paths.push_back(path);
		} catch ( const std::exception & e) {
			cerr << "Error : " << e.what() << endl;
		}
	} else if (recursive && fs::is_directory(path)) {
		for (fs::directory_iterator dir(path); dir != fs::directory_iterator(); ++dir) {
			doParsing(dir->path(), reps, paths);
		}
	}
}

void doGeneration(Gen * gen, const vector<InterRep> & reps, const vector<fs::path> & paths, const fs::path & outPathH,
                  const fs::path & outPathC)
{
	try {
		prepareOutDir(outPathH);
		prepareOutDir(outPathC);
	} catch ( const std::exception & e) {
		cerr << "Error : " << e.what() << endl;
	}
	vector<InterRep>::const_iterator itRep = reps.begin();
	vector<fs::path>::const_iterator itPath = paths.begin();
	for (itRep, itPath; itRep != reps.end() && itPath != paths.end(); itRep++, itPath++) {
		try {
			const string fileName = itPath->stem().string();
			gen->genHeader(outPathH.string(), fileName, *itRep, ns);
			gen->genSource(outPathC.string(), fileName, *itRep, ns);
		} catch ( const std::exception & e) {
			cerr << "Error : " << e.what() << endl;
		}
	}
}

int help()
{
	cout << "#################################################" << endl;
	cout << "##################### cppXb #####################" << endl;
	cout << "#################################################" << endl;
	cout << "Generates C++ sources and headers from XSD files." << endl;
	cout << "Usage : " << endl;
	cout << "  -ns : defines the namespace (default: xsdgen)" << endl;
	cout << "  -oh : defines the headers output directory (default: gen/includes)" << endl;
	cout << "  -os : defines the sources output directory (default: gen/src)" << endl;
	cout << "  -o  : defines both the headers and sources directories" << endl;
	cout << "  -r  : looks recursively in xsd folder" << endl;
	cout << "  -f  : generate a flat representation (with no virtual inheritance)" << endl;
	cout << "  -xs : use <xs:...> instead of <xsd:...>" << endl;
	cout << " last parameter : xsd file or xsds folder" << endl;
	return 0;
}

int main(int argc, char ** argv)
{
	if (argc < 2) {
		cout << "Error : Missing source file or folder parameter." << endl;
		return help();
	}
	boost::scoped_ptr<Gen> gen;
	const string src(argv[argc - 1]);
	if (src == "-h" || src == "--help") {
		return help();
	}
	cout << "Source : " << src << endl;
	if (argc > 2) {
		for (int i = 1; i < argc - 1; i++) {
			const string arg(argv[i]);
			if (arg == "-ns") {
				if (i == argc - 1) {
					return fatal("Missing namespace value for parameter 'namespace'.");
				}
				i++;
				ns = argv[i];
			} else if (arg == "-o") {
				if (i == argc - 1) {
					return fatal("Missing folder value for parameter 'out dir'.");
				}
				i++;
				outDirH = outDirC = argv[i];
			} else if (arg == "-oh") {
				if (i == argc - 1) {
					return fatal("Missing folder value for parameter 'header out dir'.");
				}
				i++;
				outDirH = argv[i];
			} else if (arg == "-os") {
				if (i == argc - 1) {
					return fatal("Missing folder value for parameter 'source out dir'.");
				}
				i++;
				outDirC = argv[i];
			} else if (arg == "-r") {
				recursive = true;
			} else if (arg == "-xs") {
				setXSDNameSpace("xs:");
			} else if (arg == "-f" && !gen) {
				gen.reset(new GenFlat());
			} else if (arg == "-h") {
				return help();
			}
		}
	}
	cout << "Namespace : " << ns << endl;
	cout << "Headers Out Dir : " << outDirH << endl;
	cout << "Sources Out Dir : " << outDirC << endl;
	if (gen) {
		cout << "Generating simplified flat representation" << endl;
	} else {
		gen.reset(new Gen());
		cout << "Generating full virtual inheritance representation" << endl;
	}

	fs::path srcPath(src);
	vector<InterRep> reps;
	vector<fs::path> paths;
	doParsing(srcPath, reps, paths);
	link(reps);
	if (reps.size() != paths.size() || reps.empty()) {
		return fatal("Invalid source : " + src);
	}
	doGeneration(gen.get(), reps, paths, fs::path(outDirH), fs::path(outDirC));
	return 0;
}
