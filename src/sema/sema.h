#ifndef LYTHON_SEMA_HEADER
#define LYTHON_SEMA_HEADER

#include "ast/magic.h"
#include "ast/ops.h"
#include "ast/visitor.h"
#include "sema/bindings.h"
#include "sema/builtin.h"
#include "sema/errors.h"
#include "utilities/strings.h"

#define SEMA_ERROR(exception)      \
    error("{}", exception.what()); \
    errors.push_back(std::unique_ptr<SemaException>(new exception));

namespace lython {

Array<String> python_paths();

struct SemaVisitorTrait {
    using StmtRet = TypeExpr*;
    using ExprRet = TypeExpr*;
    using ModRet  = TypeExpr*;
    using PatRet  = TypeExpr*;
    using IsConst = std::false_type;
};

/* The semantic analysis (SEM-A) happens after the parsing, the AST can be assumed to be
 * syntactically correct its job is to detect issues that could prevent a succesful compilation.
 *
 * Errors caught in that process are undeclared variables and mistypings,
 * this includes missing attributes, missing methods
 *
 * In addition, our SEM-A will deduce types (i.e variables inherit the type of the expressions,
 * this is NOT type inference) and allocate a register to each variables.
 *
 * To support type deduction SEM-A returns the type of the analysed expression,
 * the deduction can then be used for typechecking.
 *
 * Type deduction is a weaker form of type inference where the type of the parent parent
 * expression is deduced from the children. In the future we might add full type inference. Type
 * deduction will still be useful then as it will reduce the cost of type inference for the
 * trivial cases.
 *
 * Type deduction alone should provide a satisfactory development experience, as the user should
 * only have to specify the type of the arguments which is good practice anyway as it serves as
 * documentation.
 *
 * Notes
 * -----
 *
 * SEM-A does a quick first pass through the module to insert definitions
 * that are used before their definitions. This allow us to get away
 * with not forward-declaring everything, but it means some analytics
 * will get delayed until the end. In the case of mutually recursive definitions
 * forward declaration is required so typing can be checked.
 *
 * SEM-A will add type annotation & reorder arguments wherever it can.
 * This has the goal and standardizing the code & simplifying its execution
 * later on.
 *
 * You can inspect the change by saving the resulting AST.
 * You could implement an automatic formatter that executes semantic analysis
 * to format & complete the code. The completed code will then take less time
 * to compile as the representation will be easier to analyse.
 *
 * Raises
 * ------
 *
 * TypeError
 *      Raised when types between expression mismatch
 *
 * UnsupportedOperand
 *      Raised when using an operand on an object that does not support it
 *
 * AttributeError
 *      Raised when using an object attribute that does not exist
 *
 * NameError
 *      Raised when using an undefined variable
 *
 * ModuleNotFoundError
 *      Raised when importing a module that was not found
 *
 * ImportError
 *      Raised when importing a statement that was not found from a module
 *
 */
struct SemanticAnalyser: BaseVisitor<SemanticAnalyser, false, SemaVisitorTrait> {
    Bindings                              bindings;
    bool                                  forwardpass = false;
    Array<std::unique_ptr<SemaException>> errors;
    Array<StmtNode*>                      nested;
    Array<String>                         namespaces;
    Dict<StringRef, bool>                 flags;
    Array<String>                         paths = python_paths();

    public:
    virtual ~SemanticAnalyser() {}

    StmtNode* current_namespace() {
        if (nested.size() > 0) {
            return nested[nested.size() - 1];
        }
        return nullptr;
    }

    Tuple<ClassDef*, FunctionDef*> find_method(TypeExpr* class_type, String const& methodname);

    bool typecheck(
        ExprNode* lhs, TypeExpr* lhs_t, ExprNode* rhs, TypeExpr* rhs_t, CodeLocation const& loc);

    bool add_name(ExprNode* expr, ExprNode* value, ExprNode* type);

    String operator_function(TypeExpr* expr_t, String op);

    TypeExpr* oneof(Array<TypeExpr*> types) {
        if (types.size() > 0) {
            return types[0];
        }
        return nullptr;
    }

    ExprNode* make_ref(Node* parent, String const& name) {
        auto ref   = parent->new_object<Name>();
        ref->id    = name;
        ref->varid = bindings.get_varid(ref->id);
        return ref;
    }

    TypeExpr* resolve_variable(ExprNode* node);

    TypeExpr* attribute_assign(Attribute* n, int depth, TypeExpr* expected);

    void add_arguments(Arguments& args, Arrow*, ClassDef* def, int);

#define FUNCTION_GEN(name, fun) virtual TypeExpr* fun(name* n, int depth);

#define X(name, _)
#define SSECTION(name)
#define MOD(name, fun)   FUNCTION_GEN(name, fun)
#define EXPR(name, fun)  FUNCTION_GEN(name, fun)
#define STMT(name, fun)  FUNCTION_GEN(name, fun)
#define MATCH(name, fun) FUNCTION_GEN(name, fun)

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