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

	// save ASTs of included sources as comma separated list in m_expectations
	// TODO, only save the ASTS, separated by commas, not the stuff in between
	ifstream file(m_astFilename);
	if (!file)
		BOOST_THROW_EXCEPTION(runtime_error("Cannot open test contract: \"" + _filename + "\"."));
	file.exceptions(ios::badbit);

	if (file)
	{
		string line;
		while (getline(file, line))
			m_expectation += line + "\n";
	}

	file.close();
}

TestCase::TestResult ASTImportTest::run(ostream& _stream, string const& _linePrefix, bool const _formatted)
{
	CompilerStack c;

	c.reset(false);
	c.setEVMVersion(dev::test::Options::get().evmVersion());

	//use the compiler's analyzer to annotate, typecheck, etc...
	if (!c.importASTs(m_sourceJsons))
	{
		astAssert(false, "Import of the AST failed");
		return TestResult::FatalError;
	}
	if (!c.analyze())
	{
		astAssert(false, "Analysis of the AST failed");
		return TestResult::FatalError;
	}
	// TODO correctly report error like below
	//	else
//	{
//		SourceReferenceFormatterHuman formatter(_stream, _formatted);
//		for (auto const& error: c.errors())
//			formatter.printErrorInformation(*error);
//		return TestResult::FatalError;
//	}

	// export analyzed AST again as result
	size_t j = 0;
	for (auto const& srcPair: m_sourceJsons) // aka <string, Json>
	{
		ostringstream result;
		ASTJsonConverter(false, m_sourceIndices).print(result, c.ast(srcPair.first));
		m_result += result.str();
		if (j != m_sourceJsons.size() - 1)
		{
			m_result += ",";
		}
		m_result += "\n";
		j++;
	}

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
	for (auto const& source: m_sources)
	{
		if (m_sources.size() > 1 || source.first != "a")
			_stream << _linePrefix << "// ---- SOURCE: " << source.first << endl << endl;
		stringstream stream(source.second);
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
