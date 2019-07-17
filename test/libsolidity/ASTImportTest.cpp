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
//using namespace dev::test;
using namespace dev;
using namespace std;
namespace fs = boost::filesystem;
using namespace boost::unit_test;

ASTImportTest::ASTImportTest(string const& _filename)
{
	// TODO figure out where this is given a .sol file as input and change it to be the JSON
	if (!boost::algorithm::ends_with(_filename, ".sol"))
		BOOST_THROW_EXCEPTION(runtime_error("Invalid test contract file name: \"" + _filename + "\"."));

	// use .json as input file for the test
	m_astFilename = _filename.substr(0, _filename.size() - 4) + ".json";

	// try parsing as json
	Json::Value* ast = new Json::Value();
	// meanwhile set sourceIndices map so that ASTJsonConverter knows which index to give in SourceLocations
	size_t i = 0;
	if (jsonParseFile(m_astFilename, *ast))
	{
		if ((*ast).isMember("sourceList") && (*ast).isMember("sources"))
		{
			for (auto& src: (*ast)["sourceList"])
			{
				astAssert( (*ast)["sources"][src.asString()]["AST"]["nodeType"].asString() == "SourceUnit",  "Top-level node should be a 'SourceUnit'");
				m_sourceJsons[src.asString()] = &((*ast)["sources"][src.asString()]["AST"]);
				m_sourceIndices[src.asString()] = i;
				i++;
			}
		}
		else
		{
			BOOST_THROW_EXCEPTION(InvalidAstError() << errinfo_comment("should have been option A")); //TODO make this helpful
		}
	}
	else
	{
		BOOST_THROW_EXCEPTION(InvalidAstError() << errinfo_comment("Input file could not be parsed to JSON"));
	}

	// save entire Json from input as string in m_expectations
	// TODO but modify the metadata to not includes the hash of the solidity-source code
	// TODO2: byteCodeSansMetadata might need to be modified to remove metadata of imported contracts too
	if ( (*ast).isMember("contracts") )
	{
		for (auto& srcName: (*ast)["contracts"].getMemberNames())
		{
			// dummy
			string binSansMetadata = "0x00";
			// why is this not importing correctly? HELP
			// string binSansMetadata = dev::test::bytecodeSansMetadata((*ast)["contracts"][srcName]["bin"].asString());
			(*ast)["contracts"][srcName]["bin"] = binSansMetadata;
		}
	}
	m_expectation = dev::jsonPrettyPrint(*ast);

	// Workaround: save versionString to manually paste it into result
	m_version = (*ast)["version"].asString();
}

TestCase::TestResult ASTImportTest::run(ostream& _stream, string const& _linePrefix, bool const _formatted)
{
	CompilerStack c;

	c.reset(false);
	// TODO get EVM version from AST
	c.setEVMVersion(dev::test::Options::get().evmVersion());

	//import & use the compiler's analyzer to annotate, typecheck, etc...
	if (!c.importASTs(m_sourceJsons))
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
	if (!c.compile()) // those last two ifs can be combined into one
	{
		SourceReferenceFormatterHuman formatter(_stream, _formatted);
		for (auto const& error: c.errors())
			formatter.printErrorInformation(*error);
		return TestResult::FatalError;
	}

	// export analyzed AST again as result
	// similar to how its done when creating combined json in CommandLineInterface's handleCombinedJson()
	Json::Value output(Json::objectValue);

	// TODO use the version from the input JSON
	output["version"] = m_version; //dev::solidity::VersionString;

	// sourceList info
	output["sourceList"] = Json::Value(Json::arrayValue);
	for (auto const& source: c.sourceNames())
		output["sourceList"].append(source);

	// handle contracts-object (binary, metadata...)
	vector<string> contracts = c.contractNames();
	if (!contracts.empty())
	{
		output["contracts"] = Json::Value(Json::objectValue);
		for (string const& contractName: contracts)
		{
			Json::Value& contractData = output["contracts"][contractName] = Json::objectValue;
			string binSansMetadata = "0x00";// dev::test::bytecodeSansMetadata(c.object(contractName).toHex()); // TODO see above
			contractData["bin"] = binSansMetadata;
		}
	}
	output["sources"] = Json::Value(Json::objectValue);
	for (auto const& sourceCode: m_sourceJsons)
	{
		ASTJsonConverter converter(false, c.sourceIndices());
		output["sources"][sourceCode.first] = Json::Value(Json::objectValue);
		output["sources"][sourceCode.first]["AST"] = converter.toJson(c.ast(sourceCode.first));
	}

	// save as result
	m_result = dev::jsonPrettyPrint(output);

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
