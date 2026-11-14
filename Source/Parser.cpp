#include "Parser.h"

#include <cctype>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace ruuna {
namespace {

enum class Type {
    Int,
    String,
    Bool
};

struct Expression {
    Type type;
    std::string code;
};

struct Variable {
    Type type;
    bool mutableValue;
    std::string cName;
};

std::string cStringLiteral(const std::string& value) {
    std::string literal = "\"";
    for (unsigned char byte : value) {
        literal.push_back('\\');
        literal.push_back(static_cast<char>('0' + ((byte >> 6) & 7)));
        literal.push_back(static_cast<char>('0' + ((byte >> 3) & 7)));
        literal.push_back(static_cast<char>('0' + (byte & 7)));
    }
    literal.push_back('"');
    return literal;
}

class Parser {
public:
    explicit Parser(const std::string& source) : source_(source) {}

    std::vector<std::string> parse() {
        std::vector<std::string> statements;
        skipTrivia();
        while (!atEnd()) {
            statements.push_back(parseStatement());
            if (current() == ';') {
                advance();
            }
            skipTrivia();
        }
        return statements;
    }

private:
    const std::string& source_;
    std::size_t position_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;
    std::size_t nextVariableId_ = 0;
    std::unordered_map<std::string, Variable> variables_;

    bool atEnd() const {
        return position_ >= source_.size();
    }

    char current() const {
        return atEnd() ? '\0' : source_[position_];
    }

    void advance() {
        if (atEnd()) {
            return;
        }
        if (source_[position_++] == '\n') {
            ++line_;
            column_ = 1;
        } else {
            ++column_;
        }
    }

    [[noreturn]] void fail(const std::string& message) const {
        throw std::runtime_error(
            "line " + std::to_string(line_) + ", column " +
            std::to_string(column_) + ": " + message);
    }

    void skipTrivia() {
        while (!atEnd()) {
            if (std::isspace(static_cast<unsigned char>(current()))) {
                advance();
                continue;
            }
            if (source_.compare(position_, 2, "//") == 0 ||
                source_.compare(position_, 2, "--") == 0) {
                while (!atEnd() && current() != '\n') {
                    advance();
                }
                continue;
            }
            break;
        }
    }

    bool isIdentifierStart(char value) const {
        return std::isalpha(static_cast<unsigned char>(value)) ||
               value == '_';
    }

    bool isIdentifierPart(char value) const {
        return std::isalnum(static_cast<unsigned char>(value)) ||
               value == '_';
    }

    std::string parseIdentifier() {
        if (!isIdentifierStart(current())) {
            fail("expected an identifier");
        }
        std::string identifier;
        while (!atEnd() && isIdentifierPart(current())) {
            identifier.push_back(current());
            advance();
        }
        return identifier;
    }

    void expect(char value, const std::string& description) {
        if (current() != value) {
            fail("expected " + description);
        }
        advance();
    }

    Type parseType() {
        const std::string name = parseIdentifier();
        if (name == "Int") {
            return Type::Int;
        }
        if (name == "String") {
            return Type::String;
        }
        if (name == "Bool") {
            return Type::Bool;
        }
        fail("unknown type '" + name + "' (supported: Int, String, Bool)");
    }

    std::string typeName(Type type) const {
        switch (type) {
            case Type::Int: return "Int";
            case Type::String: return "String";
            case Type::Bool: return "Bool";
        }
        return "unknown";
    }

    Expression parseString() {
        expect('"', "a string literal");
        std::string value;
        while (!atEnd() && current() != '"') {
            if (current() == '\n' || current() == '\r') {
                fail("newline in string literal; use \\n instead");
            }
            if (current() != '\\') {
                value.push_back(current());
                advance();
                continue;
            }

            advance();
            if (atEnd()) {
                fail("unterminated escape sequence");
            }
            switch (current()) {
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                case '\\': value.push_back('\\'); break;
                case '"': value.push_back('"'); break;
                default: fail("unsupported escape sequence");
            }
            advance();
        }
        expect('"', "closing quote");
        return {Type::String, cStringLiteral(value)};
    }

    Expression parseInteger() {
        const std::size_t start = position_;
        while (std::isdigit(static_cast<unsigned char>(current()))) {
            advance();
        }
        const std::string value = source_.substr(start, position_ - start);
        try {
            std::size_t parsed = 0;
            std::stoll(value, &parsed);
            if (parsed != value.size()) {
                fail("invalid integer literal");
            }
        } catch (const std::out_of_range&) {
            fail("integer literal is out of range for Int");
        }
        if (isIdentifierStart(current())) {
            fail("invalid character after integer literal");
        }
        return {Type::Int, "(" + value + "LL)"};
    }

    Expression parsePrimary() {
        skipTrivia();
        if (current() == '"') {
            return parseString();
        }
        if (std::isdigit(static_cast<unsigned char>(current()))) {
            return parseInteger();
        }
        if (current() == '(') {
            advance();
            Expression expression = parseExpression();
            skipTrivia();
            expect(')', "')'");
            return expression;
        }
        if (isIdentifierStart(current())) {
            const std::string name = parseIdentifier();
            if (name == "tostring") {
                skipTrivia();
                expect('(', "'(' after tostring");
                Expression value = parseExpression();
                skipTrivia();
                expect(')', "')' after tostring argument");
                if (value.type != Type::Int) {
                    fail("tostring(...) requires an Int argument");
                }
                return {Type::String, "ruuna_tostring(" + value.code + ")"};
            }
            if (name == "true") {
                return {Type::Bool, "1"};
            }
            if (name == "false") {
                return {Type::Bool, "0"};
            }
            const auto variable = variables_.find(name);
            if (variable == variables_.end()) {
                fail("unknown variable '" + name + "'");
            }
            return {variable->second.type, variable->second.cName};
        }
        fail("expected a value (Int, String, Bool, or variable)");
    }

    Expression parseUnary() {
        skipTrivia();
        if (current() == '+' || current() == '-') {
            const char operation = current();
            advance();
            Expression operand = parseUnary();
            if (operand.type != Type::Int) {
                fail("unary '" + std::string(1, operation) +
                     "' requires an Int operand");
            }
            return {Type::Int,
                    "(" + std::string(1, operation) + operand.code + ")"};
        }
        return parsePrimary();
    }

    Expression parseMultiplicative() {
        Expression expression = parseUnary();
        skipTrivia();
        while (current() == '*' || current() == '/' || current() == '%') {
            const char operation = current();
            advance();
            Expression right = parseUnary();
            if (expression.type != Type::Int || right.type != Type::Int) {
                fail("arithmetic operators require Int operands");
            }
            std::string code;
            if (operation == '/') {
                code = "ruuna_divide(" + expression.code + ", " +
                       right.code + ")";
            } else if (operation == '%') {
                code = "ruuna_remainder(" + expression.code + ", " +
                       right.code + ")";
            } else {
                code = "(" + expression.code + " * " + right.code + ")";
            }
            expression = {Type::Int, code};
            skipTrivia();
        }
        return expression;
    }

    Expression parseAdditive() {
        Expression expression = parseMultiplicative();
        skipTrivia();
        while (current() == '+' || current() == '-') {
            const char operation = current();
            advance();
            Expression right = parseMultiplicative();
            if (expression.type != Type::Int || right.type != Type::Int) {
                fail("arithmetic operators require Int operands");
            }
            expression = {
                Type::Int,
                "(" + expression.code + " " + operation + " " +
                    right.code + ")"};
            skipTrivia();
        }
        return expression;
    }

    Expression parseExpression() {
        Expression expression = parseAdditive();
        skipTrivia();
        while (source_.compare(position_, 2, "..") == 0) {
            if (expression.type != Type::String) {
                fail("operator '..' requires String operands");
            }
            advance();
            advance();
            Expression right = parseAdditive();
            if (right.type != Type::String) {
                fail("operator '..' requires String operands");
            }
            expression.code =
                "ruuna_concat(" + expression.code + ", " + right.code + ")";
            skipTrivia();
        }
        return expression;
    }

    std::string parseDeclaration(bool mutableValue) {
        skipTrivia();
        const std::string name = parseIdentifier();
        if (variables_.find(name) != variables_.end()) {
            fail("variable '" + name + "' is already declared");
        }

        skipTrivia();
        bool hasDeclaredType = false;
        Type declaredType = Type::Int;
        if (current() == ':') {
            advance();
            skipTrivia();
            declaredType = parseType();
            hasDeclaredType = true;
        }

        skipTrivia();
        expect('=', "'=' in variable declaration");
        Expression initialValue = parseExpression();
        if (hasDeclaredType && declaredType != initialValue.type) {
            fail("cannot assign " + typeName(initialValue.type) +
                 " to " + typeName(declaredType) + " variable '" + name +
                 "'");
        }

        const Type type = hasDeclaredType ? declaredType : initialValue.type;
        const std::string cName =
            "ruuna_var_" + std::to_string(nextVariableId_++);
        variables_.emplace(name, Variable{type, mutableValue, cName});

        std::string declaration;
        switch (type) {
            case Type::Int:
                declaration = mutableValue ? "long long " : "const long long ";
                break;
            case Type::String:
                declaration = mutableValue ? "const char * " : "const char * const ";
                break;
            case Type::Bool:
                declaration = mutableValue ? "int " : "const int ";
                break;
        }
        return declaration + cName + " = " + initialValue.code + ";";
    }

    std::string parseAssignment(const std::string& name) {
        const auto variable = variables_.find(name);
        if (variable == variables_.end()) {
            fail("unknown variable '" + name + "'");
        }
        if (!variable->second.mutableValue) {
            fail("cannot assign to immutable val '" + name + "'");
        }

        skipTrivia();
        expect('=', "'=' in assignment");
        Expression value = parseExpression();
        if (value.type != variable->second.type) {
            fail("cannot assign " + typeName(value.type) + " to " +
                 typeName(variable->second.type) + " variable '" + name +
                 "'");
        }
        return variable->second.cName + " = " + value.code + ";";
    }

    std::string parsePrint() {
        skipTrivia();
        expect('(', "'(' after print");
        skipTrivia();
        if (current() == ')') {
            advance();
            return "fputc('\\n', stdout);";
        }

        Expression value = parseExpression();
        skipTrivia();
        expect(')', "')' after print argument");
        switch (value.type) {
            case Type::Int:
                return "printf(\"%lld\\n\", " + value.code + ");";
            case Type::String:
                return "fputs(" + value.code +
                       ", stdout); fputc('\\n', stdout);";
            case Type::Bool:
                return "puts(" + value.code + " ? \"true\" : \"false\");";
        }
        fail("unsupported print value");
    }

    std::string parseStatement() {
        skipTrivia();
        const std::string name = parseIdentifier();
        if (name == "val" || name == "var") {
            return parseDeclaration(name == "var");
        }
        if (name == "print") {
            return parsePrint();
        }
        return parseAssignment(name);
    }
};

}  // namespace

std::vector<std::string> parse(const std::string& source) {
    return Parser(source).parse();
}

}  // namespace ruuna
