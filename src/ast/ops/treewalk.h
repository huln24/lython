#pragma once

#include "ast/nodes.h"
#include "ast/visitor.h"

namespace lython {

/*
 * Simply walk the AST
 */
template <typename Implementation, bool isConst, typename VisitorTrait, typename... Args>
struct TreeWalk: public BaseVisitor<TreeWalk<Implementation, isConst, VisitorTrait, Args...>, isConst, VisitorTrait, Args...> {
    using Super = BaseVisitor<TreeWalk<Implementation, isConst, VisitorTrait, Args...>, isConst, VisitorTrait, Args...>;
    using StmtRet = typename Super::StmtRet;
    using ExprRet = typename Super::ExprRet;
    using ModRet  = typename Super::ModRet;
    using PatRet  = typename Super::PatRet;

    Array<GCObject*> parents;
    Array<Array<StmtNode*>*> body_stack;

#define TYPE_GEN(rtype, _) \
    using rtype##_t = typename Super::rtype##_t;

    KW_FOREACH_ALL(TYPE_GEN)

#undef TYPE_GEN

    #define KW_REPLACE(node, member, depth, ...)     \
        replace(node, member, depth, __VA_ARGS__)

    #define KW_COPY(dest, source, ...)\
        copy(dest, source, depth, args...)

    template<typename T, typename A>
    bool replace(T* node, A** original, int depth) {
        A* newer = exec(original[0], depth);

        if (original[0] == newer) {
            return false;
        }

        node->remove_child_if_parent(original[0], false);
        original[0] = newer;
        node->add_child(newer);
        return true;
    }

    template<typename T>
    struct ScopedOwner {
        ScopedOwner(T* owner, TreeWalk* self=nullptr):
            self(self), owner(owner)
        {
            if (self) {
                self->parents.push_back(owner);
            }
        }

        ~ScopedOwner() {
            if (self) {
                self->parents.pop_back();
            }
        }

        operator T*()  {
            return owner;
        }

        TreeWalk* self;
        T* owner;
    };

    GCObject* get_arena() {
        return (*parents.rbegin());
    }

    template<typename T>
    ScopedOwner<T> new_from(T* original, int depth, Args... args) {
        return ScopedOwner<T>(get_arena()->new_object<T>(), this);
    }

    template<typename T>
    ScopedOwner<T> new_object() {
        return ScopedOwner<T>(get_arena()->new_object<T>(), this);
    }

    template<typename T>
    ScopedOwner<T> copy(T* original, int depth, Args... args) {
        static bool deep_copy = true;
        if (deep_copy) {
            return new_from(original, depth, args...);
        }
        return ScopedOwner<T>(original, nullptr);
    }

    // template<typename T>
    // void copy(T& dest, T& source, int depth, Args... args) {
    //     dest = cast<std::remove_pointer_t<T>>(exec(source, depth, args...));
    // }
    template<typename T>
    void copy(T*& dest, T*& source, int depth, Args... args) {
        dest = cast<T>(exec(source, depth, args...));
    }

    template<typename T>
    void copy(Optional<T>& dest, Optional<T>& source, int depth, Args... args) {
        if (source.has_value()) {
            dest = some(exec(source.value(), depth, args...));
        }
    }

    CmpOperator copy(CmpOperator op, int depth, Args... args) { return op; }
    Comprehension copy(Comprehension op, int depth, Args... args) { return op; }
    Keyword copy(Keyword op, int depth, Args... args) { return op; }
    StringRef copy(StringRef op, int depth, Args... args) { return op; }

    void body_append(StmtNode* stmt) {
        (*body_stack.rbegin())->push_back(stmt);
    }

    template<typename T>
    void copy(Array<T>& dest, Array<T> & source, int depth, Args... args) {
        if constexpr (std::is_same_v<T, StmtNode*>) {
            body_stack.push_back(&dest);
        }
        if (dest != source) {
            // Copy array
            dest.reserve(source.size());
            for (int i = 0; i < source.size(); i++) {
                dest.push_back(copy((source)[i], depth, args...));
            }
        } else {
            // modify array
            for (int i = 0; i < source.size(); i++) {
                (dest)[i] = (copy((source)[i], depth, args...));
            }
        }
        if constexpr (std::is_same_v<T, StmtNode*>) {
            body_stack.pop();
        }
    }

    // Literal
    ExprRet dictexpr(DictExpr_t* n, int depth, Args... args) {
        DictExpr_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->keys, n->keys  );
        KW_COPY(cpy->values, n->values);
        return cpy;
    }
    ExprRet setexpr(SetExpr_t* n, int depth, Args... args) {
        SetExpr_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->elts, n->elts  );
        return cpy;
    }
    ExprRet listexpr(ListExpr_t* n, int depth, Args... args) {
        ListExpr_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->elts, n->elts);
        return cpy;
    }
    ExprRet tupleexpr(TupleExpr_t* n, int depth, Args... args) {
        TupleExpr_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->elts, n->elts);
        return cpy;
    }
    ExprRet constant(Constant_t* n, int depth, Args... args) { 
        return n; 
    }

    // Comprehension
    ExprRet generateexpr(GeneratorExp_t* n, int depth, Args... args) {
        GeneratorExp_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->generators, n->generators);
        KW_COPY(cpy->elt, n->elt);
        return cpy;
    }
    ExprRet listcomp(ListComp_t* n, int depth, Args... args) {
        ListComp_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->generators, n->generators);
        KW_COPY(cpy->elt, n->elt);
        return cpy;
    }
    ExprRet setcomp(SetComp_t* n, int depth, Args... args) {
        SetComp_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->generators, n->generators);
        KW_COPY(cpy->elt, n->elt);
        return cpy;
    }
    ExprRet dictcomp(DictComp_t* n, int depth, Args... args) {
        DictComp_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->generators, n->generators);
        KW_COPY(cpy->key, n->key);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }

    // Expression
    ExprRet call(Call_t* n, int depth, Args... args) {
        Call_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->func, n->func);
        KW_COPY(cpy->args, n->args);
        KW_COPY(cpy->varargs, n->varargs);
        KW_COPY(cpy->keywords, n->keywords);
        return cpy;
    }

    ExprRet namedexpr(NamedExpr_t* n, int depth, Args... args) {
        NamedExpr_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->target, n->target);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }
    ExprRet boolop(BoolOp_t* n, int depth, Args... args) {
        BoolOp_t* cpy = copy(n, depth, args...);
        cpy->op = n->op;
        KW_COPY(cpy->values, n->values);
        return cpy;
    }

    ExprRet compare(Compare_t* n, int depth, Args... args) {
        Compare_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->left, n->left);
        KW_COPY(cpy->ops, n->ops);
        KW_COPY(cpy->comparators, n->comparators);
        return cpy;
    }
    ExprRet binop(BinOp_t* n, int depth, Args... args) {
        BinOp_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->left, n->left);
        KW_COPY(cpy->right, n->right);
        return cpy;
    }
    ExprRet unaryop(UnaryOp_t* n, int depth, Args... args) {
        UnaryOp_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->operand, n->operand);
        return cpy;
    }
    ExprRet lambda(Lambda_t* n, int depth, Args... args) {
        Lambda_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->body, n->body);
        return cpy;
    }
    ExprRet ifexp(IfExp_t* n, int depth, Args... args) {
        IfExp_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->test, n->test);
        KW_COPY(cpy->body, n->body);
        KW_COPY(cpy->orelse, n->orelse);
        return cpy;
    }

    //
    ExprRet await(Await_t* n, int depth, Args... args) {
        Await_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }
    ExprRet yield(Yield_t* n, int depth, Args... args) {
        Yield_t* cpy = copy(n, depth, args...);
        KW_COPY(n->value, n->value);
        return cpy;
    }
    ExprRet yieldfrom(YieldFrom_t* n, int depth, Args... args) {
        YieldFrom_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }
    ExprRet joinedstr(JoinedStr_t* n, int depth, Args... args) {
        JoinedStr_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->values, n->values);
        return cpy;
    }
    ExprRet formattedvalue(FormattedValue_t* n, int depth, Args... args) {
        FormattedValue_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->value, n->value);
        KW_COPY(cpy->format_spec, n->format_spec);
        return cpy;
    }
    ExprRet attribute(Attribute_t* n, int depth, Args... args) {
        Attribute_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }
    ExprRet subscript(Subscript_t* n, int depth, Args... args) {
        Subscript_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->value, n->value);
        KW_COPY(cpy->slice, n->slice);
        return cpy;
    }
    ExprRet starred(Starred_t* n, int depth, Args... args) {
        Starred_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }
    ExprRet slice(Slice_t* n, int depth, Args... args) {
        Slice_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->lower, n->lower);
        KW_COPY(cpy->upper, n->upper);
        KW_COPY(cpy->step, n->step);
        return cpy;
    }

    ExprRet name(Name_t* n, int depth, Args... args) { 
        Name_t* cpy = copy(n, depth, args...);
        cpy->id = n->id;
        cpy->ctx = n->ctx;
        KW_COPY(cpy->type, n->type);
        return cpy; 
    }

    // Types
    ExprRet dicttype(DictType_t* n, int depth, Args... args) {
        DictType_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->key, n->key);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }
    ExprRet arraytype(ArrayType_t* n, int depth, Args... args) {
        ArrayType_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }
    ExprRet arrow(Arrow_t* n, int depth, Args... args) {
        Arrow_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->args, n->args);
        KW_COPY(cpy->returns, n->returns);
        return cpy;
    }
    ExprRet builtintype(BuiltinType_t* n, int depth, Args... args) { 
        BuiltinType_t* cpy = copy(n, depth, args...);
        cpy->name = n->name;
        return cpy; 
    }
    ExprRet tupletype(TupleType_t* n, int depth, Args... args) {
        TupleType_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->types, n->types);
        return cpy;
    }
    ExprRet settype(SetType_t* n, int depth, Args... args) {
        SetType_t* cpy = copy(n, depth, args...);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }
    ExprRet classtype(ClassType_t* n, int depth, Args... args) { 
        ClassType_t* cpy = copy(n, depth, args...);
        return cpy; 
    }
    ExprRet comment(Comment_t* n, int depth, Args... args) { 
        Comment_t* cpy = copy(n, depth, args...);
        return cpy; 
    }

    // JUMP

    // Leaves
    StmtRet invalidstmt(InvalidStatement_t* n, int depth, Args... args) {
        kwerror(outlog(), "Invalid statement");
        return cpy;
    }
    StmtRet returnstmt(Return_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }
    StmtRet deletestmt(Delete_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->targets, n->targets);
        return cpy;
    }
    StmtRet assign(Assign_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->targets, n->targets);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }
    StmtRet augassign(AugAssign_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->target, n->target);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }
    StmtRet annassign(AnnAssign_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->target, n->target);
        KW_COPY(cpy->annotation, n->annotation);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }
    StmtRet exprstmt(Expr_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }
    StmtRet pass(Pass_t* n, int depth, Args... args) { 
        auto* cpy = copy(n, depth, args...);
        return cpy; 
    }
    StmtRet breakstmt(Break_t* n, int depth, Args... args) { 
        auto* cpy = copy(n, depth, args...);
        return cpy; 
    }
    StmtRet continuestmt(Continue_t* n, int depth, Args... args) { 
        auto* cpy = copy(n, depth, args...);
        return cpy; 
    }
    StmtRet assertstmt(Assert_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->test, n->test);
        KW_COPY(cpy->msg, n->msg);
        return cpy;
    }
    StmtRet raise(Raise_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->exc, n->exc);
        KW_COPY(cpy->cause, n->cause);
        return cpy;
    }
    StmtRet global(Global_t* n, int depth, Args... args) { 
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->names, n->names);
        return cpy; 
    }
    StmtRet nonlocal(Nonlocal_t* n, int depth, Args... args) { 
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->names, n->names);
        return cpy; 
    }
    StmtRet import(Import_t* n, int depth, Args... args) { 
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->names, n->names);
        return cpy; 
    }
    StmtRet importfrom(ImportFrom_t* n, int depth, Args... args) { 
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->module, n->module);
        KW_COPY(cpy->names, n->names);
        KW_COPY(cpy->level, n->level);
        return cpy; 
    }

    StmtRet inlinestmt(Inline_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->body, n->body);
        return cpy;
    }
    StmtRet functiondef(FunctionDef_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->decorator_list, n->decorator_list);
        KW_COPY(cpy->args, n->args);
        KW_COPY(cpy->returns, n->returns)
        KW_COPY(cpy->body, n->body)
        return cpy;
    }
    StmtRet classdef(ClassDef_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->decorator_list, n->decorator_list);
        KW_COPY(cpy->bases, n->bases);
        KW_COPY(cpy->body, n->body);
        return cpy;
    }

    StmtRet forstmt(For_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->target, n->target);
        KW_COPY(cpy->iter, n->iter);
        KW_COPY(cpy->body, n->body);
        KW_COPY(cpy->orelse, n->orelse);
        return cpy;
    }

    StmtRet whilestmt(While_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->test, n->test);
        KW_COPY(cpy->body, n->body);
        KW_COPY(cpy->orelse, n->orelse);
        return cpy;
    }

    StmtRet ifstmt(If_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->test, n->test);
        KW_COPY(cpy->body, n->body);
        KW_COPY(cpy->orelse, n->orelse);
        return cpy;
    }

    StmtRet with(With_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->items, n->items)
        KW_COPY(cpy->body, n->body);
        return cpy;
    }
    StmtRet trystmt(Try_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->body, n->body);
        KW_COPY(cpy->handlers, n->handlers);
        KW_COPY(cpy->orlese, n->orelse);
        KW_COPY(cpy->finalbody, n->finalbody);
        return cpy;
    }

    StmtRet match(Match_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->subject, n->subject);
        KW_COPY(cpy->cases, n->cases);
        return cpy;
    }

    PatRet matchvalue(MatchValue_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->value, n->value);
        return cpy;
    }
    PatRet matchsingleton(MatchSingleton_t* n, int depth, Args... args) { 
        auto* cpy = copy(n, depth, args...);
        return cpy; 
    }
    PatRet matchsequence(MatchSequence_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->patterns, n->patterns);
        return cpy;
    }
    PatRet matchmapping(MatchMapping_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->keys, n->keys);
        KW_COPY(cpy->patterns, n->patterns);
        return cpy;
    }
    PatRet matchclass(MatchClass_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->cls, n->cls);
        KW_COPY(cpy->patterns, n->patterns);
        KW_COPY(cpy->kwd_patterns, n->kwd_patterns);
        return cpy;
    }
    PatRet matchstar(MatchStar_t* n, int depth, Args... args) { 
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->name, n->name);
        return cpy; 
    }
    PatRet matchas(MatchAs_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->pattern, n->pattern);
        KW_COPY(cpy->name, n->name);
        return cpy;
    }
    PatRet matchor(MatchOr_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->patterns, n->patterns);
        return cpy;
    }

    ModRet module(Module_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->body, n->body);
        return cpy;
    };
    ModRet interactive(Interactive_t* n, int depth, Args... args) {
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->body, n->body);
        return cpy;
    }
    ModRet functiontype(FunctionType_t* n, int depth, Args... args) { 
        auto* cpy = copy(n, depth, args...);
        return cpy; 
    }

    ModRet expression(Expression_t* n, int depth, Args... args) { 
        auto* cpy = copy(n, depth, args...);
        KW_COPY(cpy->body, n->body);
        return cpy; 
    }
};

}  // namespace lython