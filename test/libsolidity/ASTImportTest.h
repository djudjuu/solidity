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

#pragma once

#include <libdevcore/AnsiColorized.h>
#include <test/TestCase.h>

#include <iosfwd>
#include <string>
#include <vector>
#include <utility>

#include <libdevcore/JSON.h>

namespace dev
{
namespace solidity
{
namespace test
{

class ASTImportTest: public TestCase
{
public:
	static std::unique_ptr<TestCase> create(Config const& _config)
	{ return std::unique_ptr<TestCase>(new ASTImportTest(_config.filename)); }
	ASTImportTest(std::string const& _filename);

	TestResult run(std::ostream& _stream, std::string const& _linePrefix = "", bool const _formatted = false) override;

	void printSource(std::ostream& _stream, std::string const& _linePrefix = "", bool const _formatted = false) const override;
	void printUpdatedExpectations(std::ostream& _stream, std::string const& _linePrefix) const override;
private:
	// sourceName->AST how it needs to be given to the compiler
	std::map<std::string, Json::Value const*> m_sourceJsons;
	// used to tell the ASTJsonConverter how to index the sources
	std::map<std::string, unsigned> m_sourceIndices;
	std::string m_expectation;
	std::string m_astFilename;
	std::string m_result;
	// workaround
	std::string m_version;
};

}
}
}
