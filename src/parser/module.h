﻿#pragma once

#include <sstream>
#include <unordered_set>

#include "logging/logging.h"
#include "dtypes.h"
#include "ast/expressions.h"
#include "ast/nodes.h"

// This is made to indicate if the mapped value was set or not
struct Index{
    int val;

    Index(int value = -1):
        val(value)
    {}

    Index(size_t value):
        val(int(value))
    {}

    operator size_t(){
        assert(val >= 0, "");
        return size_t(val);
    }

    operator int(){
        return val;
    }

    operator bool(){ return val >= 0; }

    template<typename T> bool operator < (T i) { return val < int(i);}
    template<typename T> bool operator > (T i) { return val > int(i);}
    template<typename T> bool operator== (T i) { return val == int(i);}
    template<typename T> bool operator!= (T i) { return val != int(i);}
    template<typename T> Index operator+ (T i) const {
        return Index(val + int(i));
    }
    template<typename T>
    Index& operator+= (T i) {
        val += i;
        return *this;
    }
    template<typename T>
    Index& operator-= (T i) {
        val -= i;
        return *this;
    }
    Index& operator++ () {
        val += 1;
        return *this;
    }
    Index& operator-- () {
        val -= 1;
        return *this;
    }
};

template <typename T>
bool operator < (T i, Index a){ return a > i; }


namespace lython {
//  Module
// -------------------------------------

// Holds Currently defined entities

// I want AExpression to be hashable
struct expr_hash {
    std::size_t operator()(const Expression &v) const noexcept;
    std::hash<std::string> _h;
};

struct expr_equal {
    bool operator()(const Expression &a, const Expression &b) const noexcept;
};

// ---
/*
 * Basic Module, used by the parser to keep track of every definition
 * and make sure everything is reachable from a given scope.
 * It only saves the Name and the Expression / Type corresponding
 *
 * Evaluation use a different kind of scope.
 */
class Module {
  public:
    static Expression type_type() {
        static auto type = Expression::make<AST::Type>("Type");
        return type;
    }

    static Expression float_type() {
        static auto type =
            Expression::make<AST::Builtin>("Float", type_type(), 1);
        return type;
    }

    Module(Module const* parent = nullptr, int depth = 0, int offset = 0):
        depth(depth), offset(offset), _parent(parent)
    {
        if (!parent){
            insert("Type", type_type());
            insert("Float", float_type());

            auto make_binary = [&](){
                auto f_f_f = Expression::make<AST::Arrow>();
                auto binary_type = f_f_f.ref<AST::Arrow>();
                binary_type->params.push_back(AST::Parameter("a", reference("Float")));
                binary_type->params.push_back(AST::Parameter("b", reference("Float")));
                return f_f_f;
            };

            auto make_unary = [&](){
                auto f_f = Expression::make<AST::Arrow>();
                auto unary_type = f_f.ref<AST::Arrow>();
                unary_type->params.push_back(AST::Parameter("a", reference("Float")));
                return f_f;
            };

            auto min_fun = Expression::make<AST::Builtin>("min", make_binary(), 2);
            insert("min", Expression(min_fun));

            auto max_fun = Expression::make<AST::Builtin>("max", make_binary(), 2);
            insert("max", Expression(max_fun));

            auto sin_fun = Expression::make<AST::Builtin>("sin", make_unary(), 1);
            insert("sin", Expression(sin_fun));

            auto pi = Expression::make<AST::Value>(3.14, reference("Float"));
            insert("pi", Expression(pi));
        }
    }

    Index size() const {
        return Index(offset) + _scope.size();
    }

    Module enter() const {
        auto m = Module(this, this->depth + 1, size());
        return m;
    }

    // make a reference from a name
    Expression reference(String const &view) const{
        int tsize = size();
        int idx = _find_index(view);
        return Expression::make<AST::Reference>(view, tsize - idx, tsize, Expression());
    }

    Index insert(String const& name, Expression const& expr){
        // info("Inserting Expressionession");
        auto idx = _scope.size();
        _name_idx[name] = idx;
        _idx_name.push_back(name);
        _scope.push_back(expr);

        return idx;
    }

    Expression operator[](int idx) const {
        return get_item(idx);
    }
    Expression get_item(int idx) const {
        if (idx >= offset){
            return _scope[size_t(idx - offset)];
        }
        if (_parent){
            return _parent->get_item(idx);
        }
        return Expression();
    }

    String get_name(int idx) const {
        if (idx >= offset){
            return _idx_name[idx];
        }
        if (_parent){
            return _parent->get_name(idx);
        }
        return "nullptr";
    }

    Index _find_index(String const &view) const {
        // Check in the current scope
        auto iter = _name_idx.find(view);
        Index idx = -1;

        if (iter != _name_idx.end()){
            idx = (*iter).second + offset;
        } else if (_parent){
            idx = _parent->_find_index(view);
        }

        return int(idx);
    }

    template<typename T>
    T replace(T const& t, char a, T const& b) const{
        Index n = t.size();
        auto iter = t.rbegin();
        while (*iter == '\n'){
            n -= 1;
            iter -= 1;
        }

        int count = 0;
        for (Index i = 0; i < n; ++i){
            if (t[i] == a){
                count += 1;
            }
        }

        auto str = T(n + b.size() * size_t(count), ' ');
        Index k = 0;
        for(Index i = 0; i < n; ++i){
            if (t[i] != a){
                str[k] = t[i];
                k += 1;
            } else {
                for(Index j = 0; j < b.size(); ++j){
                    str[k] = b[j];
                    k += 1;
                }
            }
        }

        return str;
    }

    std::ostream& print(std::ostream& out, int depth = 0) const;

    class ModuleIterator{
    public:
        ModuleIterator(Module const& iter, std::size_t index = 0):
            iter(iter), index(int(index))
        {}

        bool operator==(ModuleIterator& iter){
            return this->index == iter.index;
        }

        bool operator!=(ModuleIterator& iter){
            return this->index != iter.index;
        }

        operator bool() const {
            return std::size_t(this->index) == this->iter._scope.size();
        }

        std::pair<String, Expression> operator*(){
            return {
                iter._idx_name[std::size_t(index)],
                iter._scope[std::size_t(index)]
            };
        }

        std::pair<String, Expression> operator*() const{
            return {
                iter._idx_name[std::size_t(index)],
                iter._scope[std::size_t(index)]
            };
        }

        ModuleIterator& operator++(){
            index += 1;
            return *this;
        }

        ModuleIterator& operator--(){
            index = std::max(index - 1, 0);
            return *this;
        }

    private:
        Module const& iter;
        int index;
    };

    ModuleIterator       begin()        { return ModuleIterator(*this); }
    ModuleIterator const begin() const  { return ModuleIterator(*this); }
    ModuleIterator       end()          { return ModuleIterator(*this, _scope.size()); }
    ModuleIterator const end() const    { return ModuleIterator(*this, _scope.size()); }

  private:
    //
    int const depth;
    int const offset = 0;
    Module const* _parent = nullptr;

    // stored in an array so we can do lookup by index
    Array<Expression>   _scope;
    Array<String>       _idx_name;
    Dict<String, Index> _name_idx;
};

} // namespace lython
