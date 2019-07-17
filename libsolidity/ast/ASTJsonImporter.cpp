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
/**
 * @author julius <djudju@protonmail.com>
 * @date 2017
 * Converts an AST from json format to an ASTNode.
 */

#include <libsolidity/ast/ASTJsonImporter.h>
#include <liblangutil/Scanner.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <liblangutil/Token.h>
//#include <libsolidity/inlineasm/AsmParser.h>
#include <liblangutil/SourceLocation.h>
#include <liblangutil/Exceptions.h>
#include <liblangutil/ErrorReporter.h>


using namespace std;

using namespace dev;
using namespace solidity;

using SourceLocation = langutil::SourceLocation;

template<class T>
ASTPointer<T> castPointer(ASTPointer<ASTNode> _ast)
{
	ASTPointer<T> ret = dynamic_pointer_cast<T>(_ast);
	astAssert(ret, "");
	return ret;
}

// ============ public ===========================

ASTJsonImporter::ASTJsonImporter(map<string, Json::Value const*> _sourceList )
		: m_sourceList(_sourceList)
{
	for (auto const& src: _sourceList)
		m_sourceLocations.emplace_back(make_shared<string const>(src.first));
}

map<string, ASTPointer<SourceUnit>> ASTJsonImporter::jsonToSourceUnit()
{
	for (auto const& srcPair: m_sourceList)
	{
		astAssert(!srcPair.second->isNull(), "");
		astAssert(member(*srcPair.second,"nodeType") == "SourceUnit", "The 'nodeType' of the highest node must be 'SourceUnit'.");
		m_currentSourceName = srcPair.first;
		m_sourceUnits[srcPair.first] =  createSourceUnit(*srcPair.second, srcPair.first);
	}
	return m_sourceUnits;
}

// ============ private ===========================

// =========== general creation functions ==============
template <typename T, typename... Args>
ASTPointer<T> ASTJsonImporter::createASTNode(Json::Value const& _node, Args&&... _args)
{
	astAssert(!member(_node, "id").isNull(), "'id'-field must not be null.");
	auto n = make_shared<T>(createSourceLocation(_node), forward<Args>(_args)...);
	n->setID(_node["id"].asInt());
	return n;
}

SourceLocation const ASTJsonImporter::createSourceLocation(Json::Value const& _node)
{
	astAssert(!member(_node, "src").isNull(), "'src' can not be null");
	string srcString = _node["src"].asString();
	vector<string> pos;
	boost::algorithm::split(pos, srcString, boost::is_any_of(":"));
	if (pos.size() != 3 || int(m_sourceLocations.size()) < stoi(pos[2]))
		BOOST_THROW_EXCEPTION(langutil::InvalidAstError() << errinfo_comment("'src'-field ill-formatted or src-index too high"));
	int start = stoi(pos[0]);
	int end = start + stoi(pos[1]);
	// ASSUMPTION: only the name of source is used from here on, the m_source of the CharStream-Object can be empty
	std::shared_ptr<langutil::CharStream> source = make_shared<langutil::CharStream>("", m_currentSourceName);
	return SourceLocation{ start, end,source};
}

ASTPointer<ASTNode> ASTJsonImporter::convertJsonToASTNode(Json::Value const& _json)
{
	astAssert(_json.isMember("nodeType") && _json.isMember("id"), "JSON-Node needs to have 'nodeType' and 'id' fields.");
	string nodeType = _json["nodeType"].asString();
	if (nodeType == "PragmaDirective")
		return createPragmaDirective(_json);
//	if (nodeType == "ImportDirective")
//		return createImportDirective(_json);
	if (nodeType == "ContractDefinition")
		return createContractDefinition(_json);
	if (nodeType == "InheritanceSpecifier")
		return createInheritanceSpecifier(_json);
//	if (nodeType == "UsingForDirective")
//		return createUsingForDirective(_json);
//	if (nodeType == "StructDefinition")
//		return createStructDefinition(_json);
//	if (nodeType == "EnumDefinition")
//		return createEnumDefinition(_json);
//	if (nodeType == "EnumValue")
//		return createEnumValue(_json);
//	if (nodeType == "ParameterList")
//		return createParameterList(_json);
//	if (nodeType == "FunctionDefinition")
//		return createFunctionDefinition(_json);
//	if (nodeType == "VariableDeclaration")
//		return createVariableDeclaration(_json);
//	if (nodeType == "ModifierDefinition")
//		return createModifierDefinition(_json);
//	if (nodeType == "ModifierInvocation")
//		return createModifierInvocation(_json);
//	if (nodeType == "EventDefinition")
//		return createEventDefinition(_json);
//	if (nodeType == "ElementaryTypeName")
//		return createElementaryTypeName(_json);
	if (nodeType == "UserDefinedTypeName")
		return createUserDefinedTypeName(_json);
//	if (nodeType == "FunctionTypeName")
//		return createFunctionTypeName(_json);
//	if (nodeType == "Mapping")
//		return createMapping(_json);
//	if (nodeType == "ArrayTypeName")
//		return createArrayTypeName(_json);
//	if (_json["nodeType"] == "InlineAssembly")
//		return createInlineAssembly(_json);
//	if (nodeType == "Block")
//		return createBlock(_json);
//	if (nodeType == "PlaceholderStatement")
//		return createPlaceholderStatement(_json);
//	if (nodeType == "IfStatement")
//		return createIfStatement(_json);
//	if (nodeType == "WhileStatement")
//		return createWhileStatement(_json, false);
//	if (nodeType == "DoWhileStatement")
//		return createWhileStatement(_json, true);
//	if (nodeType == "ForStatement")
//		return createForStatement(_json);
//	if (nodeType == "Continue")
//		return createContinue(_json);
//	if (nodeType == "Break")
//		return createBreak(_json);
//	if (nodeType == "Return")
//		return createReturn(_json);
//	if (nodeType == "Throw")
//		return createThrow(_json);
//	if (nodeType == "VariableDeclarationStatement")
//		return createVariableDeclarationStatement(_json);
//	if (nodeType == "ExpressionStatement")
//		return createExpressionStatement(_json);
//	if (nodeType == "Conditional")
//		return createConditional(_json);
//	if (nodeType == "Assignment")
//		return createAssignment(_json);
//	if (nodeType == "TupleExpression")
//		return createTupleExpression(_json);
//	if (nodeType == "UnaryOperation")
//		return createUnaryOperation(_json);
//	if (nodeType == "BinaryOperation")
//		return createBinaryOperation(_json);
//	if (nodeType == "FunctionCall")
//		return createFunctionCall(_json);
//	if (nodeType == "NewExpression")
//		return createNewExpression(_json);
//	if (nodeType == "MemberAccess")
//		return createMemberAccess(_json);
//	if (nodeType == "IndexAccess")
//		return createIndexAccess(_json);
//	if (nodeType == "Identifier")
//		return createIdentifier(_json);
//	if (nodeType == "ElementaryTypeNameExpression")
//		return createElementaryTypeNameExpression(_json);
//	if (nodeType == "Literal")
//		return createLiteral(_json);
	else
		BOOST_THROW_EXCEPTION(langutil::InvalidAstError() << errinfo_comment("Unknown type of ASTNode."));
}

// ============ functions to instantiate the AST-Nodes from Json-Nodes ==============

ASTPointer<SourceUnit> ASTJsonImporter::createSourceUnit(Json::Value const& _node, string const& _srcName)
{
	vector<ASTPointer<ASTNode>> nodes;
	for (auto& child: member(_node, "nodes"))
		nodes.emplace_back(convertJsonToASTNode(child));
	ASTPointer<SourceUnit> tmp = createASTNode<SourceUnit>(_node, nodes);
	tmp->annotation().path = _srcName;
	return tmp;
}

ASTPointer<PragmaDirective> ASTJsonImporter::createPragmaDirective(Json::Value const& _node)
{
	vector<Token> tokens;
	vector<ASTString> literals;
	for (auto const& lit: member(_node, "literals"))
	{
		string l = lit.asString();
		literals.push_back(l);
		tokens.push_back(scanSingleToken(l));
	}
	return createASTNode<PragmaDirective>(_node, tokens, literals);
}


//ASTPointer<ImportDirective> ASTJsonImporter::createImportDirective(Json::Value const& _node){
//	ASTPointer<ASTString> unitAlias = memberAsASTString(_node, "unitAlias");
//	ASTPointer<ASTString> path = memberAsASTString(_node, "file");
//	vector<pair<ASTPointer<Identifier>, ASTPointer<ASTString>>> symbolAliases;
//	for (auto& tuple: _node["symbolAliases"])
//	{
//                symbolAliases.push_back( make_pair(
//			createIdentifier(tuple["foreign"]),
//			tuple["local"].isNull() ? nullptr : make_shared<ASTString>(tuple["local"].asString())
//		));
//	}
//	ASTPointer<ImportDirective> tmp = createASTNode<ImportDirective>(
//		_node,
//		path,
//		unitAlias,
//		move(symbolAliases)
//	);
//	tmp->annotation().absolutePath = _node["absolutePath"].asString();
//	return tmp;
//	return nullptr;
//}

ASTPointer<ContractDefinition> ASTJsonImporter::createContractDefinition(Json::Value const& _node){
	std::vector<ASTPointer<InheritanceSpecifier>> baseContracts;
	for (auto& base : _node["baseContracts"])
		baseContracts.push_back(createInheritanceSpecifier(base));
	std::vector<ASTPointer<ASTNode>> subNodes;
	for (auto& subnode : _node["nodes"])
		subNodes.push_back(convertJsonToASTNode(subnode));
	return createASTNode<ContractDefinition>(
		_node,
		make_shared<ASTString>(_node["name"].asString()),
		nullOrASTString(_node, "documentation"),
		baseContracts,
		subNodes,
		contractKind(_node)
	);
}

ASTPointer<InheritanceSpecifier> ASTJsonImporter::createInheritanceSpecifier(Json::Value const& _node)
{
	std::vector<ASTPointer<Expression>> arguments;
	for (auto& arg : _node["arguments"])
		arguments.push_back(castPointer<Expression>(convertJsonToASTNode(arg)));
	return createASTNode<InheritanceSpecifier>(
		_node,
		createUserDefinedTypeName(_node["baseName"]),
		make_unique<std::vector<ASTPointer<Expression>>>(arguments)
	);
}

ASTPointer<UserDefinedTypeName> ASTJsonImporter::createUserDefinedTypeName(Json::Value const& _node)
{
	vector<ASTString> namePath;
	vector<string> strs;
	string nameString = member(_node, "name").asString();
	boost::algorithm::split(strs, nameString, boost::is_any_of("."));
	for (string s : strs)
		namePath.push_back(ASTString(s));
	return createASTNode<UserDefinedTypeName>(
		_node,
		namePath
	);
}


// ===== helper functions ==========

Json::Value ASTJsonImporter::member(Json::Value const& _node, string const& _name)
{
	astAssert(_node.isMember(_name), "Node '" + _node["nodeType"].asString() + "' (id " + _node["id"].asString() + ") is missing field '" + _name + "'.");
	return _node[_name];
}

Token ASTJsonImporter::scanSingleToken(Json::Value _node)
{
	langutil::Scanner scanner{langutil::CharStream(_node.asString(), "")};
	astAssert(scanner.peekNextToken() == Token::EOS, "Token string is too long.");
	return scanner.currentToken();
}

ASTPointer<ASTString> ASTJsonImporter::nullOrASTString(Json::Value _json, string const& _name){
	return _json[_name].isString() ? memberAsASTString(_json, _name) : nullptr;
}

ASTPointer<ASTString> ASTJsonImporter::memberAsASTString(Json::Value const& _node, string const& _name)
{
	Json::Value value = member(_node, _name);
	astAssert(value.isString(), "field " + _name + " must be of type string.");
	return make_shared<ASTString>(_node[_name].asString());
}

bool ASTJsonImporter::memberAsBool(Json::Value const& _node, string const& _name)
{
	Json::Value value = member(_node, _name);
	astAssert(value.isBool(), "field " + _name + " must be of type boolean.");
	return _node[_name].asBool();
}


// =========== definitions =======================
ContractDefinition::ContractKind ASTJsonImporter::contractKind(Json::Value const& _node)
{
	ContractDefinition::ContractKind kind;
	astAssert(!member(_node, "contractKind").isNull(), "'Contract-kind' can not be null.");
	if (_node["contractKind"].asString() == "interface")
		kind = ContractDefinition::ContractKind::Interface;
	else if (_node["contractKind"].asString() == "contract")
		kind = ContractDefinition::ContractKind::Contract;
	else if (_node["contractKind"].asString() == "library")
		kind = ContractDefinition::ContractKind::Library;
	else
		astAssert(false, "Unknown ContractKind");
	return kind;
}
