#ifndef LYTHON_CPP_GEN_HEADER
#define LYTHON_CPP_GEN_HEADER

#include "ast/magic.h"
#include "ast/ops.h"
#include "ast/visitor.h"
#include "sema/bindings.h"
#include "sema/builtin.h"
#include "sema/errors.h"
#include "utilities/strings.h"
#include "tide/convert/graph.h"

namespace lython {

struct ToGraphVisitorTrait {
    using StmtRet = GraphNodePinBase*;   
    using ExprRet = GraphNodePinBase*;  // Expression returns a single output pin
    using ModRet  = void;
    using PatRet  = void;
    using IsConst = std::false_type;
    using Trace   = std::true_type;

    enum
    { MaxRecursionDepth = LY_MAX_VISITOR_RECURSION_DEPTH };
};


struct Arena {
    GCObject root;
};

struct ToGraph: BaseVisitor<ToGraph, false, ToGraphVisitorTrait> {
    using Super = BaseVisitor<ToGraph, false, ToGraphVisitorTrait>;

    using StmtRet = Super::StmtRet;
    using ExprRet = Super::ExprRet;
    using ModRet  = Super::ModRet;
    using PatRet  = Super::PatRet;

    template <typename T, typename... Args>
    T* new_object(Args&&... args) {
        return arena.root.new_object<T>(args...);
    }
    Arena arena;

    GraphNodePinBase* new_input(GraphNodeBase* Node, ExprNode* expr, int depth) {
        GraphNodePinBase* in = new_object<GraphNodePin>(); 
        in->direction() = PinDirection::Input;
        in->kind() = PinKind::Data;
        in->pins().push_back(exec(expr, depth));
        Node->pins().push_back(in);
        return in;
    }

    GraphNodePinBase* new_output(GraphNodeBase* Node, int depth) {
        GraphNodePinBase* out = new_object<GraphNodePin>(); 
        out->direction() = PinDirection::Output;
        out->kind() = PinKind::Data;
        Node->pins().push_back(out);
        // current_exec_pin = out;
        return out;
    }

    GraphNodePinBase* new_exec_input(GraphNodeBase* Node, int depth) {
        GraphNodePinBase* in = new_object<GraphNodePin>(); 
        in->direction() = PinDirection::Input;
        in->kind() = PinKind::Exec;
        Node->pins().push_back(in);

        if (current_exec_pin) {
            current_exec_pin->pins().push_back(in);
            current_exec_pin = nullptr;
        }
        return in;
    }

    GraphNodePinBase* new_exec_output(GraphNodeBase* Node, int depth) {
        GraphNodePinBase* out = new_object<GraphNodePin>(); 
        out->direction() = PinDirection::Output;
        out->kind() = PinKind::Exec;
        Node->pins().push_back(out);
        return out;
    }
#define TYPE_GEN(rtype) using rtype##_t = Super::rtype##_t;

#define X(name, _)
#define SSECTION(name)
#define EXPR(name, fun)  TYPE_GEN(name)
#define STMT(name, fun)  TYPE_GEN(name)
#define MOD(name, fun)   TYPE_GEN(name)
#define MATCH(name, fun) TYPE_GEN(name)

    NODEKIND_ENUM(X, SSECTION, EXPR, STMT, MOD, MATCH)

#undef X
#undef SSECTION
#undef EXPR
#undef STMT
#undef MOD
#undef MATCH
#undef TYPE_GEN

    Bindings              bindings;
    bool                  forwardpass = false;
    Array<StmtNode*>      nested;
    Array<String>         namespaces;
    Dict<StringRef, bool> flags;
    GraphNodePinBase*     current_exec_pin = nullptr;

    public:
    virtual ~ToGraph() {}

#define FUNCTION_GEN(name, fun, ret) virtual ret fun(name##_t* n, int depth);

#define X(name, _)
#define SSECTION(name)
#define MOD(name, fun)   FUNCTION_GEN(name, fun, ModRet)
#define EXPR(name, fun)  FUNCTION_GEN(name, fun, ExprRet)
#define STMT(name, fun)  FUNCTION_GEN(name, fun, StmtRet)
#define MATCH(name, fun) FUNCTION_GEN(name, fun, PatRet)

    NODEKIND_ENUM(X, SSECTION, EXPR, STMT, MOD, MATCH)

#undef X
#undef SSECTION
#undef EXPR
#undef STMT
#undef MOD
#undef MATCH

#undef FUNCTION_GEN
};

}  // namespace lython

#endif