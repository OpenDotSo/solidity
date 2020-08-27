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
// SPDX-License-Identifier: GPL-3.0
/**
 * Implements generators for synthesizing mostly syntactically valid
 * Solidity test programs.
 */

#pragma once

#include <test/tools/ossfuzz/Generators.def>

#include <libsolutil/Whiskers.h>

#include <liblangutil/Exceptions.h>
#include <random>
#include <set>
#include <variant>

namespace solidity::test::fuzzer
{
using RandomEngine = std::mt19937_64;
using Distribution = std::uniform_int_distribution<size_t>;

struct GenerationProbability
{
	enum class NumberLiteral
	{
		DECIMAL,
		HEX
	};

	static size_t distributionOneToN(size_t _n, std::shared_ptr<RandomEngine> _rand)
	{
		return Distribution(1, _n)(*_rand);
	}
	static bool chooseOneOfN(size_t _n, std::shared_ptr<RandomEngine> _rand)
	{
		return distributionOneToN(_n, _rand) == 1;
	}
	static std::string chooseOneOfNStrings(
		std::vector<std::string> const& _list,
		std::shared_ptr<RandomEngine> _rand
	)
	{
		return _list[GenerationProbability{}.distributionOneToN(_list.size(), _rand) - 1];
	}
	static std::string generateRandomAsciiString(size_t _length, std::shared_ptr<RandomEngine> _rand);
	static std::string generateRandomHexString(size_t _length, std::shared_ptr<RandomEngine> _rand);
	static std::pair<NumberLiteral, std::string> generateRandomNumberLiteral(size_t _length, std::shared_ptr<RandomEngine> _rand);
};

/// Forward declarations
#define SEMICOLON() ;
#define COMMA() ,
#define EMPTY()
#define FORWARDDECLAREGENERATORS(G) class G
GENERATORLIST(FORWARDDECLAREGENERATORS, SEMICOLON(), SEMICOLON())
#undef FORWARDDECLAREGENERATORS
class SolidityGenerator;
struct TestState;

/// Type declarations
using GeneratorPtr = std::variant<
#define VARIANTOFSHARED(G) std::shared_ptr<G>
GENERATORLIST(VARIANTOFSHARED, COMMA(), EMPTY())
>;
#undef VARIANTOFSHARED

using Generator = std::variant<
#define VARIANTOFGENERATOR(G) G
GENERATORLIST(VARIANTOFGENERATOR, COMMA(), EMPTY())
>;
#undef VARIANTOFGENERATOR
#undef EMPTY
#undef COMMA
#undef SEMICOLON

struct NameVisitor
{

	template <typename T>
	std::string operator()(T const& _t)
	{
		return _t->name();
	}
};

struct GeneratorVisitor
{
	template <typename T>
	std::string operator()(T const& _t)
	{
		return _t->visit();
	}
};

struct ResetVisitor
{
	template <typename T>
	void operator()(T const& _t)
	{
		_t->reset();
	}
};

struct AddDependenciesVisitor
{
	template <typename T>
	void operator()(T const& _t)
	{
		_t->setup();
	}
};

struct GeneratorBase
{
	GeneratorBase(std::shared_ptr<SolidityGenerator> _mutator);
	template <typename T>
	std::shared_ptr<T> generator()
	{
		for (auto &g: generators)
			if (std::holds_alternative<std::shared_ptr<T>>(g))
				return std::get<std::shared_ptr<T>>(g);
		solAssert(false, "");
	}
	GeneratorPtr randomGenerator();
	/// Generator
	virtual std::string visit() = 0;
	std::string visitChildren();
	void addGenerators(std::set<GeneratorPtr> _generators)
	{
		for (auto &g: _generators)
			generators.insert(g);
	}
	virtual void reset() = 0;
	virtual std::string name() = 0;
	virtual void setup() = 0;
	virtual ~GeneratorBase() {};
	std::shared_ptr<SolidityGenerator> mutator;
	/// Random engine shared by Solidity mutators
	std::shared_ptr<RandomEngine> rand;
	std::shared_ptr<TestState> state;
	std::set<GeneratorPtr> generators;
};

class IntegerTypeGenerator: public GeneratorBase
{
public:
	IntegerTypeGenerator(std::shared_ptr<SolidityGenerator> _mutator): GeneratorBase(std::move(_mutator))
	{}
	std::string visit() override;
	void setup() override {}
	void reset() override {}
	std::string name() override
	{
		return "IntegerTypeGenerator";
	}
};

class BytesTypeGenerator: public GeneratorBase
{
public:
	BytesTypeGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{}
	void setup() override;
	std::string visit() override;
	void reset() override {}
	std::string name() override
	{
		return "BytesTypeGenerator";
	}
};

class BoolTypeGenerator: public GeneratorBase
{
public:
	BoolTypeGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{}
	void setup() override {}
	std::string visit() override;
	void reset() override {}
	std::string name() override
	{
		return "BoolTypeGenerator";
	}
};

class AddressTypeGenerator: public GeneratorBase
{
public:
	AddressTypeGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{}
	void setup() override {}
	std::string visit() override;
	void reset() override {}
	std::string name() override
	{
		return "AddressTypeGenerator";
	}
};

class FunctionTypeGenerator: public GeneratorBase
{
public:
	FunctionTypeGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{}
	void setup() override;
	void reset() override {}
	std::string name() override
	{
		return "Function type generator";
	}
	std::string visit() override;
private:
	static const std::vector<std::string> s_visibility;
	std::string const m_functionTypeTemplate =
		std::string(R"(function (<paramList>) )") +
		R"(<visibility> <stateMutability>)" +
		R"(<?return> returns (<retParamList>)</return>)";
};

class UserDefinedTypeGenerator: public GeneratorBase
{
public:
	UserDefinedTypeGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{}
	void setup() override;
	std::string visit() override;
	void reset() override {}
	std::string name() override
	{
		return "User defined type generator";
	}
};

class ArrayTypeGenerator: public GeneratorBase
{
public:
	ArrayTypeGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator)),
		m_numDimensions(0)
	{}
	void setup() override;
	std::string visit() override;
	void reset() override {}
	std::string name() override
	{
		return "Array type generator";
	}
private:
	size_t m_numDimensions;
	static size_t constexpr s_maxArrayDimensions = 3;
	static size_t constexpr s_maxStaticArraySize = 5;
};

class ExpressionGenerator: public GeneratorBase
{
public:
	enum Type
	{
		INDEXACCESS = 0ul,
		INDEXRANGEACCESS,
		MEMBERACCESS,
		FUNCTIONCALLOPTIONS,
		FUNCTIONCALL,
		PAYABLECONVERSION,
		METATYPE,
		UNARYPREFIXOP,
		UNARYSUFFIXOP,
		EXPOP,
		MULDIVMODOP,
		ADDSUBOP,
		SHIFTOP,
		BITANDOP,
		BITXOROP,
		BITOROP,
		ORDERCOMPARISON,
		EQUALITYCOMPARISON,
		ANDOP,
		OROP,
		CONDITIONAL,
		ASSIGNMENT,
		NEWEXPRESSION,
		TUPLE,
		INLINEARRAY,
		IDENTIFIER,
		LITERAL,
		ELEMENTARYTYPENAME,
		USERDEFINEDTYPENAME,
		TYPEMAX
	};
	ExpressionGenerator(
		std::shared_ptr<SolidityGenerator> _mutator,
		bool _compileTimeConstantExpressionsOnly = false
	):
		GeneratorBase(std::move(_mutator)),
		m_expressionNestingDepth(0),
		m_compileTimeConstantExpressionsOnly(_compileTimeConstantExpressionsOnly)
	{}
	void setup() override;
	std::string visit() override;
	void reset() override
	{
		m_expressionNestingDepth = 0;
	}
	std::string name() override
	{
		return "Expression Generator";
	}
private:
	std::string boolLiteral()
	{
		return GenerationProbability{}.chooseOneOfN(2, rand) ? "true" : "false";
	}
	std::string doubleQuotedStringLiteral();
	std::string hexLiteral();
	std::string numberLiteral();
	std::string addressLiteral();
	std::string literal();
	std::string expression();
	void incrementNestingDepth()
	{
		m_expressionNestingDepth++;
	}
	bool nestingDepthTooHigh()
	{
		return m_expressionNestingDepth > s_maxNumNestedExpressions;
	}
	size_t m_expressionNestingDepth;
	bool m_compileTimeConstantExpressionsOnly;
	static constexpr size_t s_maxNumNestedExpressions = 5;
	static constexpr size_t s_maxStringLength = 10;
	static constexpr size_t s_maxHexLiteralLength = 64;
	static constexpr size_t s_maxElementsInTuple = 4;
	static constexpr size_t s_maxElementsInlineArray = 4;
};

class StateVariableDeclarationGenerator: public GeneratorBase
{
public:
	enum Visibility
	{
		PUBLIC = 0,
		PRIVATE,
		INTERNAL,
		VISIBILITYMAX
	};
	StateVariableDeclarationGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{}
	void setup() override;
	std::string visit() override;
	void reset() override {}
	std::string name() override
	{
		return "StateVariableDeclarationGenerator";
	}
private:
	std::string identifier()
	{
		return "sv" + std::to_string(GenerationProbability{}.distributionOneToN(s_maxStateVariables, rand));
	}
	std::string visibility();
	static constexpr size_t s_maxStateVariables = 3;
	std::string const m_declarationTemplate =
		std::string(R"(<natSpecString>)") +
		R"(<type> <vis><?constant> constant</constant><?immutable> immutable</immutable> <id> = <value>;)";
};

class TypeGenerator: public GeneratorBase
{
public:
	TypeGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator)),
		m_nonValueType(false)
	{}
	void setup() override;
	std::string visit() override;
	std::string visitNonArrayType();
	void reset() override {}
	std::string name() override
	{
		return "TypeGenerator";
	}
	bool nonValueType()
	{
		return m_nonValueType;
	}
	void setNonValueType()
	{
		m_nonValueType = true;
	}
private:
	bool m_nonValueType;
};

struct Exports
{
	Exports(std::string& _path): sourceUnitPath(_path),	symbols({}), types({})
	{}
	/// Source unit path
	std::string sourceUnitPath;
	/// Exported symbols
	std::set<std::string> symbols;
	/// Exported user defined types
	std::set<std::string> types;
};

struct ExportedSymbols
{
	ExportedSymbols(): symbols({}), types({})
	{}
	ExportedSymbols& operator+=(ExportedSymbols& _right)
	{
		for (auto item: _right.symbols)
			if (!symbols.count(item))
				symbols.emplace(item);
		for (auto item: _right.types)
			if (!types.count(item))
				types.emplace(item);
		return *this;
	}
	ExportedSymbols& operator+=(std::string& _right)
	{
		if (!symbols.count(_right))
			symbols.emplace(_right);
		if (!types.count(_right))
			types.emplace(_right);
		return *this;
	}
	std::string randomSymbol(std::shared_ptr<RandomEngine> _rand);
	std::string randomUserDefinedType(std::shared_ptr<RandomEngine> _rand);
	std::set<std::string> symbols;
	std::set<std::string> types;
};

struct SolidityType
{
	enum class Type
	{
#define TYPESTRING(X) X
#define COMMA ,
#define EMPTY
#include <test/tools/ossfuzz/Types.def>
TYPELIST(TYPESTRING, COMMA, EMPTY)
#undef COMMA
#undef EMPTY
#undef TYPESTRING
	};
	virtual ~SolidityType() {}
	std::string typeString(Type _type);
	Type type;
};

struct FunctionState
{
	/// Parameter type, name pair
	using ParamType = std::pair<SolidityType, std::string>;
	enum class Mutability
	{
		PURE,
		VIEW,
		PAYABLE,
		NONPAYABLE,
	};
	enum class Visibility
	{
		EXTERNAL,
		INTERNAL,
		PUBLIC,
		PRIVATE
	};
	enum class Inheritance
	{
		VIRTUAL,
		OVERRIDE,
		VIRTUALOVERRIDE,
		NONE
	};
	Mutability randomMutability(std::shared_ptr<RandomEngine> _rand)
	{
		switch (GenerationProbability{}.distributionOneToN(4, _rand))
		{
		case 1:
			return Mutability::PURE;
		case 2:
			return Mutability::VIEW;
		case 3:
			return Mutability::PAYABLE;
		case 4:
			return Mutability::NONPAYABLE;
		}
		solAssert(false, "");
	}
	Mutability randomFreeFunctionMutability(std::shared_ptr<RandomEngine> _rand)
	{
		switch (GenerationProbability{}.distributionOneToN(3, _rand))
		{
		case 1:
			return Mutability::PURE;
		case 2:
			return Mutability::VIEW;
		case 3:
			return Mutability::NONPAYABLE;
		}
		solAssert(false, "");
	}
	void setName(std::string _name)
	{
		name = _name;
	}
	void setMutability(Mutability _mut)
	{
		mutability = _mut;
	}
	void setVisibility(Visibility _vis)
	{
		visibility = _vis;
	}
	void setParameterTypes(std::vector<ParamType> _paramTypes)
	{
		inputParameters = std::move(_paramTypes);
	}
	void setReturnTypes(std::vector<ParamType> _returnTypes)
	{
		returnParameters = std::move(_returnTypes);
	}
	void setInheritance(Inheritance _inh)
	{
		inheritance = _inh;
	}

	bool operator==(FunctionState const& _other);
	std::string name;
	Mutability mutability;
	Visibility visibility;
	std::vector<ParamType> inputParameters;
	std::vector<ParamType> returnParameters;
	Inheritance inheritance;
};

struct SourceUnitState
{
	SourceUnitState(): exportedSymbols({})
	{}
	void exportSymbol(std::string& _symbol)
	{
		exportedSymbols += _symbol;
	}
	void exportSymbols(ExportedSymbols& _symbols)
	{
		exportedSymbols += _symbols;
	}
	void addFunction(std::shared_ptr<FunctionState> _function)
	{
		exportedSymbols += _function->name;
		functions.emplace_back(_function);
	}
	bool functionExists(std::shared_ptr<FunctionState> _function)
	{
		for (auto const& f: functions)
			if (f == _function)
				return true;
		return false;
	}
	bool symbols()
	{
		return exportedSymbols.symbols.size() > 0;
	}
	bool userDefinedTypes()
	{
		return exportedSymbols.types.size() > 0;
	}
	ExportedSymbols exportedSymbols;
	std::vector<std::shared_ptr<FunctionState>> functions;
};

struct ImportState
{
	/// Maps a symbol to its alias identifier
	using SymbolAliases = std::map<std::string, std::string>;
	/// A single alias identifier for all symbols
	using UnitAlias = std::string;
	/// An alias is optional, when present it is either
	/// a single identifier or a mapping of symbols to
	/// their respective alias identifiers.
	using Alias = std::optional<std::variant<SymbolAliases, UnitAlias>>;
	ImportState(std::string&& _path, std::set<std::string>&& _symbols, Alias _alias):
		path(_path),
		symbols(std::move(_symbols)),
		aliases(std::move(_alias))
	{}
	/// Import path
	std::string path;
	/// Imported symbols
	std::set<std::string> symbols;
	/// Alias representation
	Alias aliases;
};

struct TestState
{
	TestState(std::shared_ptr<RandomEngine> _rand):
		sourceUnitStates({}),
		currentSourceName({}),
		rand(std::move(_rand))
	{}
	void addSourceUnit(std::string& _path)
	{
		sourceUnitStates.emplace(_path, SourceUnitState{});
		currentSourceName = _path;
	}
	bool empty()
	{
		return sourceUnitStates.empty();
	}
	size_t size()
	{
		return sourceUnitStates.size();
	}
	void print();
	std::string randomPath();
	std::string randomNonCurrentPath();
	std::string currentSourceUnit()
	{
		return currentSourceName;
	}
	SourceUnitState& currentSourceState()
	{
		return sourceUnitStates[currentSourceUnit()];
	}
	std::map<std::string, SourceUnitState> sourceUnitStates;
	std::string currentSourceName;
	std::shared_ptr<RandomEngine> rand;
};

class TestCaseGenerator: public GeneratorBase
{
public:
	TestCaseGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator)),
		m_numSourceUnits(0)
	{}
	void setup() override;
	void reset() override {}
	std::string visit() override;
	std::string name() override
	{
		return "Test case generator";
	}
	bool empty()
	{
		return m_numSourceUnits == 0;
	}
	std::string randomPath();
	std::shared_ptr<TestState> testState()
	{
		return state;
	}
private:
	std::string path(unsigned _number) const
	{
		return m_sourceUnitNamePrefix + std::to_string(_number) + ".sol";
	}
	std::string path() const
	{
		return m_sourceUnitNamePrefix + std::to_string(m_numSourceUnits) + ".sol";
	}
	void addSourceUnit(std::string& _path)
	{
		state->addSourceUnit(_path);
	}

	size_t m_numSourceUnits;
	std::string const m_sourceUnitNamePrefix = "su";
	std::string const m_sourceUnitHeader = "\n" + std::string(R"(==== Source: <path> ====)") + "\n";
	static constexpr unsigned s_maxSourceUnits = 1;
};

class PragmaGenerator: public GeneratorBase
{
public:
	PragmaGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{}
	void setup() override {}
	void reset() override {}
	std::string visit() override;
	std::string name() override { return "Pragma generator"; }
private:
	std::string generateExperimentalPragma();
	const std::string m_pragmaTemplate =
		std::string(R"(pragma <version>;)") +
		"\n" +
		R"(pragma <experimental>;)";
};

class ImportGenerator: public GeneratorBase
{
public:
	ImportGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{}
	void setup() override {}
	std::string visit() override;
	void reset() override {}
	std::string name() override { return "Import generator"; }
private:
	std::vector<Exports> m_globalExports;
	std::string const m_importPathAs = R"(import "<path>"<?as> as <identifier></as>;)";
	std::string const m_importStar = R"(import * as <identifier> from "<path>";)";
	std::string const m_alias = R"(<symbol><?as> as <alias></as>)";
	std::string const m_importSymAliases = R"(import {<aliases>} from "<path>";)";
	static constexpr size_t s_selfImportInvProb = 101;
};

struct InterfaceFunction
{
//	FunctionMutability mutability;
};

struct InterfaceState
{

};

struct ContractState
{
	ContractState():
		baseContractStates({}),
		functionStates({})
	{}

	void addBaseContract();
	void addFunction();
	std::vector<std::shared_ptr<ContractState>> baseContractStates;
	std::vector<std::shared_ptr<FunctionState>> functionStates;
};

struct Expression
{
	std::string visit()
	{
		return expressionTemplate;
	}
	std::string const expressionTemplate = R"(1)";
};

struct NamedArgument
{
	std::string visit()
	{
		return identifier + ": " + expression.visit();
	}
	std::string identifier;
	Expression expression;
};

struct NamedArgumentList
{
	std::set<NamedArgument> namedArguments;
	std::string const namedTemplate = R"({<commaSepNamedArgs>})";
};

struct CallArgument
{
	using Argument = std::variant<Expression, NamedArgument>;
    Argument argument;
};

struct CallArgumentList
{
	std::vector<CallArgument> callArguments;
};

struct InheritanceSpecifier
{
	std::string name;
	std::optional<CallArgumentList> callArguments;
};

struct InheritanceSpecifierList
{
	std::set<InheritanceSpecifier> inheritanceSpecifier;
};

struct Location
{
	enum class Loc
	{
		MEMORY,
		STORAGE,
		CALLDATA,
		STACK
	};
	Location(Loc _l): loc(_l) {}
	Loc loc;
	std::string visit();
};

class LocationGenerator: public GeneratorBase
{
public:
	LocationGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{}
	void setup() override {}
	std::string visit() override;
	void reset() override {}
	std::string name() override
	{
		return "LocationGenerator";
	}
};

struct IntegerWidth
{
	IntegerWidth(unsigned _w)
	{
		width = (8 * _w) % 256;
	}
	std::string visit()
	{
		return width > 0 ? std::to_string(width) : std::string("256");
	}
	unsigned width;
};

struct IntegerType: SolidityType
{
	IntegerType(bool _signed, unsigned _width): sign(_signed), width(_width)
	{}
	bool sign;
	IntegerWidth width;
};

struct Statement
{
	virtual ~Statement() {}
	virtual std::string visit() = 0;
};

struct ExpressionStatement: Statement
{
	ExpressionStatement(Expression _expr): expression(std::move(_expr))
	{}
	std::string visit() override;
	Expression expression;
	std::string const exprStmtTemplate = R"(<expression>;)";
};

struct VariableDeclaration
{
	VariableDeclaration(std::shared_ptr<SolidityType> _type, Location _loc, std::string&& _id):
        type(std::move(_type)),
		location(_loc),
		identifier(std::move(_id))
	{}
	std::shared_ptr<SolidityType> type;
	Location location;
	std::string identifier;
	std::string visit();
	std::string const varDeclTemplate = R"(<type> <location> <name>;)";
};

class VariableDeclarationGenerator: public GeneratorBase
{
public:
	VariableDeclarationGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{}
	void setup() override;
	void reset() override {}
	std::string name() override
	{
		return "VariableDeclarationGenerator";
	}
	std::string visit() override;
private:
	std::string identifier();
};

struct ParameterListState
{

};

class ParameterListGenerator: public GeneratorBase
{
public:
	ParameterListGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{}
	void setup() override;
	void reset() override {}
	std::string visit() override;
	std::string name() override
	{
		return "ParameterListGenerator";
	}
};

struct ParameterList
{
	std::vector<VariableDeclaration> params;
	std::string const parameterListTemplate = R"(<commaSeparatedParams>)";
};

struct VariableDeclarationTuple
{
	std::vector<VariableDeclaration> varDecls;
	std::string const varDeclTupleTemplate =
		R"(<commaStarPre><varDecl><?commaStarPost><commaStar><!commaStarPost><commaSepVarDecls></commaStarPost>)";
};

struct VariableDeclarationTupleAssignment: Statement
{
	VariableDeclarationTuple tuple;
	Expression expression;
	std::string visit() override;
	std::string const varDeclTupleAssignTemplate =
		R"(<tuple> = <expression>;)";
};

struct SimpleVariableDeclaration: Statement
{
	SimpleVariableDeclaration(
		std::shared_ptr<SolidityType> _type,
		Location _loc,
		std::string&& _id,
		std::optional<std::shared_ptr<Expression>> _expr
	):
		type(std::move(_type)),
		location(_loc),
		identifier(std::move(_id)),
		expression(std::move(_expr))
	{}
	std::shared_ptr<SolidityType> type;
	Location location;
	std::string identifier;
	std::optional<std::shared_ptr<Expression>> expression;
	std::string visit() override;
	std::string const simpleVarDeclTemplate =
		R"(<type> <location> <name><?assign> = <expression></assign>;)";
};

struct GeneratorTypeVisitor
{
	template <typename T>
	std::string operator()(T& _value)
	{
		return _value.visit();
	}
};

struct VariableDeclarationStatement: Statement
{
	using VarDeclStmt = std::variant<SimpleVariableDeclaration, VariableDeclarationTupleAssignment>;
	VariableDeclarationStatement(VarDeclStmt _stmt): stmt(std::move(_stmt))
	{}
	VarDeclStmt stmt;
	std::string visit() override
	{
		return std::visit(GeneratorTypeVisitor{}, stmt);
	}
};

struct SimpleStatement: Statement
{
	using Stmt = std::variant<VariableDeclarationStatement, ExpressionStatement>;
	SimpleStatement(Stmt _stmt): statement(std::move(_stmt))
	{}
	Stmt statement;
	std::string visit() override
	{
		return std::visit(GeneratorTypeVisitor{}, statement);
	}
};

/// Forward declaration
struct BlockStatement;
using StatementTy = std::variant<SimpleStatement, BlockStatement>;

struct BlockStatement: Statement
{
	BlockStatement(std::vector<StatementTy>&& _stmts): statements(std::move(_stmts))
	{}
	std::vector<StatementTy> statements;
	std::string visit() override;
};

class EnumDeclaration: public GeneratorBase
{
public:
	EnumDeclaration(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{}
	void setup() override {}
	std::string visit() override;
	void reset() override {}
	std::string name() override
	{
		return "Enum generator";
	}
private:
	std::string enumName()
	{
		return "E" + std::to_string((*rand)() % s_maxIdentifiers);
	}
	std::string const enumTemplate = R"(enum <name> { <members> })";
	static constexpr size_t s_maxMembers = 5;
	static constexpr size_t s_maxIdentifiers = 4;
};

class ConstantVariableDeclaration: public GeneratorBase
{
public:
	ConstantVariableDeclaration(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{}
	void setup() override;
	std::string visit() override;
	void reset() override {}
	std::string name() override { return "Constant variable generator"; }
private:
	std::string const constantVarDeclTemplate =
		R"(<type> constant <name> = <expression>;)";
};

class FunctionDefinitionGenerator: public GeneratorBase
{
public:
	FunctionDefinitionGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{
		m_functionState = std::make_shared<FunctionState>();
	}
	void setup() override;
	std::string visit() override;
	void reset() override {}
	std::string name() override { return "Function generator"; }
	void freeFunctionMode()
	{
		m_freeFunction = true;
	}
	void contractFunctionMode()
	{
		m_freeFunction = false;
	}
	static const std::vector<std::string> s_mutability;
private:
	std::string functionIdentifier();
	std::shared_ptr<TestState> m_state;
	std::shared_ptr<FunctionState> m_functionState;
	bool m_freeFunction;
	static const std::vector<std::string> s_visibility;
	static const std::vector<std::string> s_freeFunctionMutability;
	std::string const m_functionTemplate =
		R"(<natSpecString>)" +
		std::string(R"(function <id> (<paramList>) )") +
		R"(<visibility> <stateMutability> <modInvocation> <virtual> <overrideSpec>)" +
		R"(<?return> returns (<retParamList>)</return>)" +
		R"(<?definition><body><!definition>;</definition>)";
};

class ContractDefinitionGenerator: public GeneratorBase
{
public:
	ContractDefinitionGenerator(std::shared_ptr<SolidityGenerator> _generator):
		GeneratorBase(std::move(_generator))
	{}
	void setup() override;
	std::string visit() override;
	void reset() override {}
	std::string name() override { return "Contract generator"; }
private:
	std::optional<InheritanceSpecifierList> m_inheritanceList;
	std::shared_ptr<BlockStatement> m_body;
	const std::string m_contractTemplate =
		R"(<natSpecString>)" +
		std::string(R"(<?abstract>abstract</abstract> contract <id>)") +
		R"(<?inheritance> is <inheritanceSpecifierList></inheritance> { <stateVar> <function> })";
	/// List of inverse probabilities of sub-components
	static constexpr size_t s_abstractInvProb = 10;
	static constexpr size_t s_inheritanceInvProb = 10;
};

class SourceUnitGenerator: public GeneratorBase
{
public:
	SourceUnitGenerator(std::shared_ptr<SolidityGenerator> _mutator):
		GeneratorBase(std::move(_mutator))
	{
		m_sourceState = std::make_shared<SourceUnitState>();
	}
	void setup() override;
	std::string visit() override;
	void reset() override;
	std::string name() override { return "Source unit generator"; }
private:
	void saveState();
	std::shared_ptr<TestState> m_testState;
	std::shared_ptr<SourceUnitState> m_sourceState;
	static constexpr unsigned s_maxElements = 10;
};

class NatSpecGenerator: public GeneratorBase
{
public:
	enum class TagCategory
	{
		CONTRACT,
		FUNCTION,
		PUBLICSTATEVAR,
		EVENT
	};
	enum class Tag
	{
		TITLE,
		AUTHOR,
		NOTICE,
		DEV,
		PARAM,
		RETURN,
		INHERITDOC
	};
	NatSpecGenerator(std::shared_ptr<SolidityGenerator> _generator): GeneratorBase(std::move(_generator))
	{
		m_nestingDepth = 0;
	}
	void setup() override {}
	std::string visit() override;
	void reset() override
	{
		m_nestingDepth = 0;
	}
	std::string name() override { return "NatSpec generator"; }
	void tagCategory(TagCategory _tag)
	{
		m_tag = _tag;
	}
private:
	std::string randomNatSpecString(TagCategory _category);
	Tag randomTag(TagCategory _category);
	TagCategory m_tag;
	size_t m_nestingDepth;
	static std::map<TagCategory, std::vector<Tag>> s_tagLookup;
	static constexpr size_t s_maxTextLength = 8;
	static constexpr size_t s_maxNestedTags = 3;
	std::string const m_tagTemplate = R"(<tag> <random> <recurse>)";
};

struct InterfaceSpecifiers
{
	std::set<std::string> typeNames;
};

struct SourceState {
	unsigned numPragmas;
	unsigned numImports;
	unsigned numContracts;
	unsigned numAbstractContracts;
	unsigned numInterfaces;
	unsigned numLibraries;
	unsigned numGlobalStructs;
	unsigned numGlobalFuncs;
	unsigned numGlobalEnums;
};

struct ProgramState
{
	enum class ContractType
	{
		CONTRACT,
		ABSTRACTCONTRACT,
		INTERFACE,
		LIBRARY
	};

	unsigned numFunctions;
	unsigned numModifiers;
	unsigned numContracts;
	unsigned numLibraries;
	unsigned numInterfaces;
	unsigned numStructs;
	unsigned numEvents;
	bool constructorDefined;
	ContractType contractType;
};

class SolidityGenerator: public std::enable_shared_from_this<SolidityGenerator>
{
public:
	explicit SolidityGenerator(unsigned int _seed);

	std::string visit();
	template <typename T>
	std::shared_ptr<T> generator();
	std::shared_ptr<RandomEngine> randomEngine()
	{
		return m_rand;
	}
	std::shared_ptr<TestState> testState()
	{
		return m_state;
	}
	std::string generateTestProgram();
private:
	template <typename T>
	void createGenerator()
	{
		m_generators.insert(
			std::make_shared<T>(shared_from_this())
		);
	}
	template <std::size_t I = 0>
	void createGenerators();

	void initialize();

	/// @returns either true or false with roughly the same probability
	bool coinToss()
	{
		return (*m_rand)() % 2 == 0;
	}
	/// @returns a pseudo randomly chosen unsigned integer between one
	/// and @param _n
	size_t randomOneToN(size_t _n)
	{
		return Distribution(1, _n)(*m_rand);
	}
	/// Random number generator
	std::shared_ptr<RandomEngine> m_rand;
	/// Sub generators
	std::set<GeneratorPtr> m_generators;
	/// Test state
	std::shared_ptr<TestState> m_state;
};
}
