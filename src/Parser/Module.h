﻿#pragma once

#include <unordered_set>

#include "../Types.h"
#include "../Utilities/trie.h"
#include "AbstractSyntaxTree/Expressions.h"

namespace lython {
//  Module
// -------------------------------------

// Holds Currently defined entities

// I want AST::Expr to be hashable
struct expr_hash {
    std::size_t operator()(const ST::Expr &v) const noexcept;
    std::hash<std::string> _h;
};

struct expr_equal {
    bool operator()(const ST::Expr &a, const ST::Expr &b) const noexcept;
};

typedef std::unordered_set<ST::Expr, expr_hash, expr_equal> BaseScope;

// ---
class Module {
  public:
    static ST::Expr type_type() {
        static ST::Expr type = std::make_shared<AST::Type>("Type");
        return type;
    }

    static ST::Expr float_type() {
        static ST::Expr type =
            std::make_shared<AST::Builtin>("Float", type_type());
        return type;
    }

    Module() {
        _module["Type"] = type_type();
        _module["Float"] = float_type();

        for (auto c : {"+", "-", "*", "/", ".*", "./", "%", "^"}) {
            _operators.insert(c);
        }

        auto min_type = new AST::Arrow();
        min_type->params.push_back(
            AST::Parameter(make_name("a"), float_type()));
        min_type->params.push_back(
            AST::Parameter(make_name("a"), float_type()));
        auto min_fun = new AST::Builtin("min", ST::Expr(min_type));

        auto max_fun = new AST::Function("max", true);
        max_fun->args().push_back(AST::Parameter(make_name("a"), float_type()));
        max_fun->args().push_back(AST::Parameter(make_name("b"), float_type()));

        auto sin_fun = new AST::Function("sin", true);
        sin_fun->args().push_back(AST::Parameter(make_name("x"), float_type()));

        _module["min"] = ST::Expr(min_fun);
        _module["sin"] = ST::Expr(sin_fun);
        _module["max"] = ST::Expr(max_fun);
    }

    Dict<String, std::tuple<int, bool>> &precedence_table() {
        return _precedence_table;
    }

    Trie<128> const *operator_trie() const { return &_operators.trie(); }

    static Dict<String, std::tuple<int, bool>> default_precedence() {
        static Dict<String, std::tuple<int, bool>> val = {
            {"+", {2, true}}, // Predecence, Left Associative
            {"-", {2, true}}, {"%", {1, true}},  {"*", {3, true}},
            {"/", {3, true}}, {".*", {2, true}}, {"./", {2, true}},
            {"^", {4, false}}};
        return val;
    }

    int arg_count(std::string_view view) const {
        thread_local String _tmp(view.begin(), view.end());
        ST::Expr fun_expr = _module.at(_tmp);

        if (fun_expr != nullptr &&
            fun_expr->kind() == AST::Expression::KindFunction) {
            auto fun = static_cast<AST::Function *>(fun_expr.get());
            return int(fun->args().size());
        }

        return -1;
    }

    ST::Expr register_struct(AST::Struct *struct_expr) {
        auto expr = _module[struct_expr->name()];
        if (expr != nullptr) {
            warn("overriding definition %s", struct_expr->name().c_str());
        }

        auto r = ST::Expr(struct_expr);
        _module[struct_expr->name()] = r;
        return r;
    }

    ST::Expr register_function(AST::Function *fun_expr) {
        auto name = fun_expr->name();
        std::string str_name(std::begin(name), std::end(name));

        auto expr = _module[str_name];
        if (expr != nullptr) {
            warn("overriding definition %s", str_name.c_str());
        }

        auto r = ST::Expr(fun_expr);
        _module[str_name] = r;
        return r;
    }

    auto begin() { return _module.begin(); }
    auto begin() const { return _module.begin(); }
    auto end() { return _module.end(); }
    auto end() const { return _module.end(); }

    ST::Expr find(std::string const &view) { return _module[view]; }

  private:
    // Functions and types
    Dict<String, ST::Expr> _module;
    // This is more for parsing
    CoWTrie<128> _operators;
    Dict<String, std::tuple<int, bool>> _precedence_table =
        default_precedence();
};
}
