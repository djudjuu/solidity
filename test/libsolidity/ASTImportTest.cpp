/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <test/libsolidity/ASTImportTest.h>
#include <test/Options.h>
#include <test/Metadata.h>
#include <libdevcore/AnsiColorized.h>
#include <liblangutil/SourceReferenceFormatterHuman.h>
#include <libsolidity/ast/ASTJsonConverter.h>
#include <libsolidity/ast/ASTJsonImporter.h>
#include <libsolidity/interface/CompilerStack.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/throw_exception.hpp>
#include <fstream>
#include <memory>
#include <stdexcept>


using namespace langutil;
using namespace dev::solidity;
using namespace dev::solidity::test;
using namespace dev::formatting;
using namespace dev::test;
using namespace dev;
using namespace std;
namespace fs = boost::filesystem;
using namespace boost::unit_test;

ASTImportTest::ASTImportTest(string const& _filename)
{
	// 1) load .sol file into compiler

	if (!boost::algorithm::ends_with(_filename, ".sol"))
		BOOST_THROW_EXCEPTION(runtime_error("Invalid test contract file name: \"" + _filename + "\"."));

	ifstream file(_filename);
	if (!file)
		BOOST_THROW_EXCEPTION(runtime_error("Cannot open test contract: \"" + _filename + "\"."));
	file.exceptions(ios::badbit);

	// ========== my version START =========
	// TODO understand what this is doing exactly
	string sourceName;
	string source;
	string line;
	string const sourceDelimiter("// ---- SOURCE: ");
	string const delimiter("// ----");

	while (getline(file, line))
	{
		if (boost::algorithm::starts_with(line, sourceDelimiter))
		{
			if (!sourceName.empty())
				m_sources[sourceName] = source;
			sourceName = line.substr(sourceDelimiter.size(), string::npos);
			source = string();
		}
		else if (!line.empty() && !boost::algorithm::starts_with(line, delimiter))
			source += line + "\n";
	}

	m_sources[sourceName.empty() ? "a" : sourceName] = source;
	// ==== my version END ===========

	file.close();

	CompilerStack c;
	c.setSources(m_sources);

	// 2) parse, analyze, compile()

	c.setEVMVersion(dev::test::Options::get().evmVersion());
	if (c.parse()) {
		c.analyze();
	}
	else
	{
		cout << "ERRORRRR" << endl;
		BOOST_THROW_EXCEPTION(runtime_error("Cannot parse and analyze test contract: \"" + _filename + "\"."));
		// TODO output compiler errors c.errors()
		//		SourceReferenceFormatterHuman formatter(_stream, _formatted);
		//		for (auto const& error: c.errors())
		//			formatter.printErrorInformation(*error);
	}
	if (!c.compile())
		BOOST_THROW_EXCEPTION(runtime_error("Could not compile test contract: \"" + _filename + "\"."));

	// 3) export AST & binary into m_expectations, such that it can
	// be compared
	// printed
	// imported into the compiler again

	m_expectationJSON = combineOutputsInJSON(&c);
	m_expectation = dev::jsonPrettyPrint(m_expectationJSON);
//	cout << "expected is saved as" << m_expectation << endl;
}

TestCase::TestResult ASTImportTest::run(ostream& _stream, string const& _linePrefix, bool const _formatted)
{
	// 4) load ast from expectations into reset compiler
	CompilerStack c;
	c.reset(true); // keep settings

	// create input for ast-import function from m_expectations
	map<string, Json::Value const*> c_import_input;
	for (auto const& source: m_sources)
	{
		c_import_input[source.first] = &m_expectationJSON["sources"][source.first]["AST"];
	}

	//import & use the compiler's analyzer to annotate, typecheck, etc...
	if (!c.importASTs(c_import_input))
	{
		// is this the correct way to report an error here?
		astAssert(false, "Import of the AST failed");
		return TestResult::FatalError;
	}
	if (!c.analyze())
	{
		SourceReferenceFormatterHuman formatter(_stream, _formatted);
		for (auto const& error: c.errors())
			formatter.printErrorInformation(*error);
		return TestResult::FatalError;
	}
	if (!c.compile())
	{
		SourceReferenceFormatterHuman formatter(_stream, _formatted);
		for (auto const& error: c.errors())
			formatter.printErrorInformation(*error);
		return TestResult::FatalError;
	}
	// export output again, but into m_results
	m_result = dev::jsonPrettyPrint(combineOutputsInJSON(&c));
//	 cout << "result is " << m_result << endl;

	// compare & error reporting
	bool resultsMatch = true;

	if (m_expectation != m_result)
	{
		string nextIndentLevel = _linePrefix + "  ";
		AnsiColorized(_stream, _formatted, {BOLD, CYAN}) << _linePrefix << "Expected result:" << endl;
		{
			istringstream stream(m_expectation);
			string line;
			while (getline(stream, line))
				_stream << nextIndentLevel << line << endl;
		}
		_stream << endl;
		AnsiColorized(_stream, _formatted, {BOLD, CYAN}) << _linePrefix << "Obtained result:" << endl;
		{
			istringstream stream(m_result);
			string line;
			while (getline(stream, line))
				_stream << nextIndentLevel << line << endl;
		}
		_stream << endl;
		resultsMatch = false;
	}

	return resultsMatch ? TestResult::Success : TestResult::Failure;
}

Json::Value ASTImportTest::combineOutputsInJSON(CompilerStack* _c)
{
	Json::Value output(Json::objectValue);

	vector<string> contracts = _c->contractNames();
	if (!contracts.empty())
	{
		// holds as many entries as there are contracts in all combined sourcefiles
		output["contracts"] = Json::Value(Json::objectValue);
		for (string const& contractName: contracts)
		{
			Json::Value& contractData = output["contracts"][contractName] = Json::objectValue;
//			string binSans = dev::test::bytecodeSansMetadata(_c->object(contractName).toHex());
//			contractData["bin"] = "";//binSans; //TODO
			// see above in line 84
//			 string binSansMetadata = dev::test::bytecodeSansAnyMetadata(c.object(contractName).toHex());
			// workaround:
			 string binSansMetadata = dev::test::bytecodeSansMetadata(dev::test::bytecodeSansMetadata(_c->object(contractName).toHex()));
			contractData["bin"] = binSansMetadata;
		}
	}
	// keep around, maybe this will be useful to resolve why the importing files break....
	// list of filepaths that hold all included contracts
	//	output["sourceList"] = Json::Value(Json::arrayValue);
	//	for (auto const& source: c.sourceNames())
	//	output["sourceList"].append(source);

	//to hold the ASTs, (as many as there are actual sourcefiles)
	output["sources"] = Json::Value(Json::objectValue);
	for (auto const& source: m_sources)
	{
		ASTJsonConverter converter(false, _c->sourceIndices());
		output["sources"][source.first] = Json::Value(Json::objectValue);
		output["sources"][source.first]["AST"] = converter.toJson(_c->ast(source.first));
	}
	return output;
}


void ASTImportTest::printSource(ostream& _stream, string const& _linePrefix, bool const) const
{
	for (auto const& source: m_sourceJsons)
	{
		if (m_sourceJsons.size() > 1 || source.first != "a")
			_stream << _linePrefix << "// ---- SOURCE: " << source.first << endl << endl;
		stringstream stream(dev::jsonPrettyPrint(source.second));
		string line;
		while (getline(stream, line))
			_stream << _linePrefix << line << endl;
		_stream << endl;
	}
}

void ASTImportTest::printUpdatedExpectations(std::ostream&, std::string const&) const
{
	ofstream file(m_astFilename.c_str());
	if (!file) BOOST_THROW_EXCEPTION(runtime_error("Cannot write AST expectation to \"" + m_astFilename + "\"."));
	file.exceptions(ios::badbit);
	file << m_result;
	file.flush();
	file.close();
}
