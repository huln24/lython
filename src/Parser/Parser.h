﻿#ifndef PARSER_H
#define PARSER_H

#include "../Lexer/Lexer.h"
#include "../Logging/logging.h"
#include "../Types.h"
#include "../Utilities/optional.h"
#include "../Utilities/stack.h"
#include "../Utilities/trie.h"

#include "Module.h"

#include <iostream>
#include <numeric>

/*
 *  TODO:
 *      Handle tok_incorrect so the parser does not stop
 *      I need a more generic way to parse tokens since
 *      I will need to parse vector<Token> too
 *
 *      Some statistic about compiler memory usage would be nice
 *
 *  if definition 1 is incorrect
 *  The parser should be able to parse definition 2 (which is correct)
 *  correctly
 *
 * if a body is incorrect it has no impact if the function is not used
 *
 *  I can't have an incorrect identifier. Only Numbers can be incorrect
 *
 */

// "" at the end of the macro remove a warning about unecessary ;
#define EAT(tok)                                                               \
    if (token().type() == tok) {                                               \
        next_token();                                                          \
    }                                                                          \
    ""

// assert(token().type() == tok && msg)
#define TRACE_START()                                                          \
    trace_start(depth, "(%s: %i)", tok_to_string(token().type()).c_str(),      \
                token().type())
#define TRACE_END()                                                            \
    trace_end(depth, "(%s: %i)", tok_to_string(token().type()).c_str(),        \
              token().type())

#define CHECK_TYPE(type) type
#define CHECK_NAME(name) name
#define PARSE_ERROR(msg) std::cout << msg

#define show_token(tok) debug(tok_to_string(tok.type()))
#define EXPECT(tok, msg)                                                       \
    ASSERT(token().type() == tok, msg);                                        \
    ""


namespace lython {

class ParserException : public std::exception {
  public:
    ParserException(String const &msg) : msg(msg) {}

    const char *what() const noexcept override { return msg.c_str(); }

    const String msg;
};

#define WITH_EXPECT(tok, msg)                                                  \
    if (token().type() != tok) {                                               \
        debug("Got (tok: %s, %d)", tok_to_string(token().type()).c_str(),      \
              token().type());                                                 \
        throw ParserException(msg);                                            \
    } else


class Parser {
  public:
    Parser(AbstractBuffer &buffer, Module *module)
        : module(module), _lex(buffer) {}

    // Shortcut
    Token next_token()  { return _lex.next_token(); }
    Token token()       { return _lex.token();      }
    Token peek_token()  { return _lex.peek_token(); }

    String get_identifier() {
        if (token().type() == tok_identifier) {
            return token().identifier();
        }

        debug("Missing identifier");
        return String("<identifier>");
    }

    /*  <function-definition> ::= {<declaration-specifier>}* <declarator>
     * {<declaration>}* <compound-statement>
     */
    ST::Expr parse_function(Module& m, std::size_t depth);

    ST::Expr parse_compound_statement(Module& m, std::size_t depth);

    Token ignore_newlines();

    ST::Expr parse_type(Module& m, std::size_t depth);

    AST::ParameterList parse_parameter_list(Module& m, std::size_t depth);

    ST::Expr parse_value(std::size_t depth) {
        TRACE_START();

        AST::Value *val = nullptr;
        int8 type = token().type();

        switch (type) {
        case tok_string:
            val = new AST::Value(token().identifier(), make_type("string"));
            EAT(tok_string);
            break;
        case tok_float:
            val = new AST::Value(token().as_float(), make_type("float"));
            EAT(tok_float);
            break;
        case tok_int:
            val = new AST::Value(token().as_integer(), make_type("int"));
            EAT(tok_int);
            break;
        }

        // are we done ?
        auto ttype = token().type();
        if (ttype == tok_newline || ttype == tok_eof || ttype == ',' ||
            ttype == ')')
            return ST::Expr(val);

        auto op = new AST::Ref();
        if (token().type() == tok_identifier) {
            op->name() = get_identifier();
            EAT(tok_identifier);
        } else {
            op->name() = token().type();
            next_token();
        }

        ST::Expr rhs = parse_value(depth + 1);
        AST::BinaryOperator *bin =
            new AST::BinaryOperator(rhs, ST::Expr(val), ST::Expr(op));
        TRACE_END();
        return ST::Expr(bin);
    }

    ST::Expr parse_statement(Module& m, int8 statement, std::size_t depth) {
        TRACE_START();
        EXPECT(statement, ": was expected");
        EAT(statement);

        auto *stmt = new AST::Statement();
        stmt->statement() = statement;

        stmt->expr() = parse_expression(m, depth + 1);
        return ST::Expr(stmt);
    }

    ST::Expr parse_function_call(Module& m, ST::Expr function, std::size_t depth) {
        TRACE_START();
        auto fun = new AST::Call();
        fun->function() = function;

        EXPECT('(', "`(` was expected");
        EAT('(');

        while (token().type() != ')') {
            // token().debug_print(std::cout);

            fun->arguments().push_back(parse_expression(m, depth + 1));

            if (token().type() == ',') {
                next_token();
            }
        }

        EXPECT(')', "`)` was expected");
        EAT(')');
        return ST::Expr(fun);
    }

    String parse_operator() {
        Trie<128> const *iter = nullptr;
        String op_name;

        // Operator is a string
        // Currently this code path is not used
        // not sure if we should allow identifier to be operators
        if (token().type() == tok_identifier) {
            iter = module->operator_trie();
            iter = iter->matching(token().identifier());

            if (iter != nullptr) {
                if (iter->leaf()) {
                    op_name = token().identifier();
                    debug("Operator is string");
                } else {
                    warn("Operator %s was not found, did you mean: ...",
                         token().identifier().c_str());
                }
            }
        } // ----
        else {
            // debug("Operator parsing");
            iter = module->operator_trie();

            bool operator_parsing = true;
            while (operator_parsing) {
                // debug("Tok is %c", char(token().type()));

                // check next token
                iter = iter->matching(token().type());

                if (iter != nullptr) {
                    // debug("Added tok to operator %c", char(token().type()));
                    op_name.push_back(token().type());
                    next_token();
                } else {
                    warn("Could not match %c %d", char(token().type()),
                         token().type());
                }

                // token that stop operator parsing
                switch (token().type()) {
                case '(':
                case ')':
                case tok_identifier:
                case tok_float:
                case tok_int:
                case tok_newline:
                    operator_parsing = false;
                    // debug("Found terminal tok %c", char(token().type()));
                    break;
                }
            }

            if (!iter->leaf()) {
                warn("Operator %s was not found, did you mean: ...",
                     op_name.c_str());
            }
        }
        return op_name;
    }

    // Shunting-yard_algorithm
    // Parse a full line of function and stuff
    ST::Expr parse_expression(Module& m, std::size_t depth);

    // parse function_name(args...)
    ST::Expr parse_top_expression(Module& m, std::size_t depth) {
        TRACE_START();

        switch (token().type()) {
        case tok_async:
        case tok_yield:
        case tok_return:
            return parse_statement(m, token().type(), depth + 1);

        case tok_def:
            return parse_function(m, depth + 1);

        // value probably an operation X + Y
        //            case tok_identifier:{
        //                AST::Ref* fun_name = new AST::Ref();
        //                fun_name->name() = token().identifier();
        //                EAT(tok_identifier);
        //                return parse_function_call(ST::Expr(fun_name), depth +
        //                1);
        //            }

        case tok_struct:
            return parse_struct(m, depth + 1);

        case tok_identifier:
        case tok_string:
        case tok_int:
        case tok_float:
            return parse_expression(m, depth + 1);

            //            default:
            //                return parse_operator(depth + 1);
        }

        return nullptr;
    }

    /*  <struct-or-union> ::= struct | union
        <struct-or-union-specifier> ::= <struct-or-union> <identifier> {
       {<struct-declaration>}+ }
                                      | <struct-or-union> {
       {<struct-declaration>}+ }
                                      | <struct-or-union> <identifier>
     */
    ST::Expr parse_struct(Module& m, std::size_t depth) {
        TRACE_START();
        EAT(tok_struct);

        Token tok = token();
        EXPECT(tok_identifier, "Expect an identifier");
        String name = tok.identifier();
        EAT(tok_identifier);

        auto *data = new AST::Struct();
        data->name() = name;

        EXPECT(':', ": was expected");
        EAT(':');
        EXPECT(tok_newline, "newline was expected");
        EAT(tok_newline);
        EXPECT(tok_indent, "indentation was expected");
        EAT(tok_indent);

        tok = token();

        // docstring
        if (tok.type() == tok_docstring) {
            data->docstring() = tok.identifier();
            tok = next_token();
        }

        while (tok.type() == tok_newline) {
            tok = next_token();
        }

        tok = token();
        while (tok.type() != tok_desindent && tok.type() != tok_eof) {

            WITH_EXPECT(tok_identifier, "Expected identifier 1") {
                name = tok.identifier();
                EAT(tok_identifier);
            }

            EXPECT(':', "Expect :");
            EAT(':');
            tok = token();

            WITH_EXPECT(tok_identifier, "Expect type identifier") {
                AST::Ref *ref = new AST::Ref();
                ref->name() = tok.identifier();
                data->attributes()[name] = ST::Expr(ref);
                EAT(tok_identifier);
                tok = token();
            }

            while (tok.type() == tok_newline) {
                tok = next_token();
            }
        }

        auto struct_ptr = ST::Expr(data);
        module->insert(name, struct_ptr);
        return struct_ptr;
    }

    // return One Top level Expression (Functions)
    ST::Expr parse_one(Module& m, std::size_t depth = 0) {
        Token tok = token();
        if (tok.type() == tok_incorrect) {
            tok = next_token();
        }

        while (tok.type() == tok_newline) {
            tok = next_token();
        }

        switch (tok.type()) {
        case tok_def:
            return parse_function(m, depth);

        case tok_struct:
            return parse_struct(m, depth);

        default:
            assert("Unknown Token");
        }

        return nullptr;
    }

    //
//    BaseScope parse_all() {
//        // first token is tok_incorrect
//        while (token()) {
//            _scope.insert(parse_one());
//        }
//    }

  private:
    // Top Level Module
    Module *module;
    Lexer _lex;
    // BaseScope _scope;
};

} // namespace lython

#endif // PARSER_H
