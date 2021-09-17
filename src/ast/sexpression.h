#ifndef LYTHON_SEXPR_HEADER
#define LYTHON_SEXPR_HEADER

#include <memory>

#include "../dtypes.h"
#include "../utilities/optional.h"

namespace lython {

using Identifier = String;


enum class ObjectKind {
    Module,
    Statement,
    Expression,
    Pattern,
};

struct GCObject {
public:
    template<typename T, typename... Args>
		T* new_object(Args&&... args) {
				auto& alloc = get_allocator<T>();

				// allocate
				T* memory = alloc.allocate(1);

				// construct
				T* obj = new((void*)memory) T(std::forward<Args>(args)...);

        children.push_back(obj);
        return obj;
    }

    //! Make an object match the lifetime of the parent
    template<typename T>
    void add_child(T* child) {
				children.push_back(child);
    }

    template<typename T, typename D>
    void add_child(Unique<T, D> child) {
        children.push_back(child.release());
    }

    void remove_child(GCObject* child, bool free) {
        auto elem = std::find(children.rbegin(), children.rend(), child);
        // This is why people hate C++
        //
        // This removes the element found by the reverse iterator.
        // Reverse iterator do not point to the found element
        // but the element before it.
        // erase is not specialized to take reverse iterator
        // so we need to convert it ourselves and KNOW that this is what is
        // expected but nobody could have guessed that
				auto n = children.size();
				children.erase(std::next(elem).base());
				assert(n > children.size(), "Child was not removed");

        if (free) {
						// FIXME: this breaks the metadata
						device::CPU().free((void*)child, 1);
        }
    }

    ~GCObject(){
        for (auto obj: children) {
						// FIXME: this breaks the metadata
						device::CPU().free((void*)obj, 1);
        }
    }

    GCObject(ObjectKind k):
        kind(k)
    {}


    ObjectKind const kind;

private:
    template<typename T>
    AllocatorCPU<T>& get_allocator() {
        static auto alloc = AllocatorCPU<T>();
        return alloc;
    }

private:
    Array<GCObject*> children;
};

struct ConstantValue {
public:
    enum Type {
        TInt,
        TFloat,
        TString
    };

    ConstantValue() = default;

    ConstantValue(int v):
        kind(TInt)
    {
        value.integer = v;
    }

    ConstantValue(float v):
        kind(TFloat)
    {
        value.decimal = v;
    }

    ConstantValue(double v):
        kind(TFloat)
    {
        value.decimal = v;
    }

    ConstantValue(String const& v):
        kind(TString)
    {
        value.string = v;
    }

    ConstantValue(ConstantValue const& v){
        copy_union(v.kind, v.value);
    }

    ~ConstantValue() {
        if (kind == TString) {
            value.string.~String();
        }
    }

    ConstantValue& operator= (ConstantValue const& v) {
        copy_union(v.kind, v.value);
        return *this;
    }

    template<typename T>
    ConstantValue& operator= (T v) {
        *this = ConstantValue(v);
        return *this;
    }

private:
    // ast.Str, ast.Bytes, ast.NameConstant, ast.Ellipsis
    union ValueVariant {
        ValueVariant()  {}
        ~ValueVariant() {}

        int integer;
        double decimal;
        String string;
    };

    ValueVariant value;

    Type kind;

    void copy_union(Type k, ValueVariant const& v) {
        kind = k;
        switch (kind) {
        case TString: value.string = v.string;
        case TFloat: value.decimal = v.decimal;
        case TInt: value.integer = v.integer;
        }
    }
};

// col_offset is the byte offset in the utf8 string the parser uses
struct CommonAttributes {
    int lineno;
    int col_offset;
    Optional<int> end_lineno;
    Optional<int> end_col_offset;
};

struct Node {
	virtual void print(std::ostream& out) {}
};

struct ModNode: public GCObject, public Node {
    ModNode():
        GCObject(ObjectKind::Module)
    {}
};

struct StmtNode: public CommonAttributes, public GCObject, public Node {
    StmtNode():
        GCObject(ObjectKind::Statement)
    {}
};

struct ExprNode: public CommonAttributes, public GCObject, public Node {
    ExprNode():
        GCObject(ObjectKind::Expression)
    {}
};

enum class ConversionKind {
    None = -1,
    String = 115,
    Representation = 114,
    ASCII = 97
};

enum class BinaryOperator  {
    Add,
    Sub,
    Mult,
    MatMult,
    Div,
    Mod,
    Pow,
    LShift,
    RShift,
    BitOr,
    BitXor,
    BitAnd,
    FloorDiv
};

enum class BoolOperator {
    And,
    Or
};

enum class UnaryOperator {
    Invert,
    Not,
    UAdd,
    USub,
};

enum class ExprContext {
    Load,
    Store,
    Del
};

enum class CmpOperator {
    Eq,
    NotEq,
    Lt,
    LtE,
    Gt,
    GtE,
    Is,
    IsNot,
    In,
    NotIn,
};

struct Comprehension {
    ExprNode* target = nullptr;
    ExprNode* iter = nullptr;
    Array<ExprNode*> ifs;
    int is_async;
};

struct ExceptHandler: public CommonAttributes {
    Optional<ExprNode*> type;
    Optional<Identifier> name;
    Array<StmtNode*> body;
};

struct Arg: public CommonAttributes {
    Identifier arg;
    Optional<ExprNode*> annotation;
    Optional<String> type_comment;
};

struct Arguments {
    Arguments() = default;

    Arguments(Arguments const&) = default;

    Array<Arg> posonlyargs;
    Array<Arg> args;
		Optional<Arg> vararg;				// *args
    Array<Arg> kwonlyargs;
    Array<ExprNode*> kw_defaults;
		Optional<Arg> kwarg;				// **kwargs
    Array<ExprNode*> defaults;
};

struct Keyword: public CommonAttributes {
    Optional<Identifier> arg;
    ExprNode* value = nullptr;
};

struct Alias {
    Identifier name;
    Optional<Identifier> asname;
};

struct WithItem {
    ExprNode* context_expr = nullptr;
    Optional<ExprNode*> optional_vars;
};

struct TypeIgnore {
    int lineno;
    String tag;
};

struct Pattern: public CommonAttributes, public GCObject {
    Pattern():
        GCObject(ObjectKind::Pattern)
    {}
};

struct MatchValue : public Pattern {
    ExprNode* value;
};

struct MatchSingleton: public Pattern {
    ConstantValue value;
};

struct MatchSequence: public Pattern {
    Array<Pattern*> patterns;
};

// The optional "rest" MatchMapping parameter handles capturing extra mapping keys
struct MatchMapping: public Pattern {
    Array<ExprNode*> keys;
    Array<Pattern*> patterns;
    Optional<Identifier> rest;
};

struct MatchClass: public Pattern {
    ExprNode* cls;
    Array<Pattern*> patterns;
    Array<Identifier> kwd_attrs;
    Array<Pattern*> kwd_patterns;
};

struct MatchStar: public Pattern {
    Optional<Identifier> name;
};

struct MatchAs: public Pattern {
    Optional<Pattern*> pattern;
    Optional<Identifier> name;
};

struct MatchOr: public Pattern {
    Array<Pattern*> patterns;
};

struct MatchCase {
    Pattern* pattern;
    Optional<ExprNode*> guard;
    Array<StmtNode*> body;
};

// Expressions
// -----------

struct BoolOp: public ExprNode{
    BoolOperator op;
    Array<ExprNode*> values;

};
struct NamedExpr: public ExprNode{
    ExprNode* target = nullptr;
    ExprNode* value = nullptr;
};

struct BinOp: public ExprNode{
    ExprNode* left = nullptr;
    BinaryOperator op;
    ExprNode* right = nullptr;
};

struct UnaryOp: public ExprNode{
    UnaryOperator op;
    ExprNode* operand;
};

struct Lambda: public ExprNode{
    Arguments args;
    ExprNode* body = nullptr;
};

struct IfExp: public ExprNode{
    ExprNode* test = nullptr;
    ExprNode* body = nullptr;
    ExprNode* orelse = nullptr;
};

struct DictExpr: public ExprNode{
    Array<ExprNode*> keys;
    Array<ExprNode*> values;
};

struct SetExpr: public ExprNode{
    Array<ExprNode*> elts;
};

struct ListComp: public ExprNode{
    ExprNode* elt = nullptr;
    Array<Comprehension> generators;
};

struct SetComp: public ExprNode{
    ExprNode* elt = nullptr;
    Array<Comprehension> generators;
};

struct DictComp: public ExprNode{
    ExprNode* key = nullptr;
    ExprNode* value = nullptr;
    Array<Comprehension> generators;
};

struct GeneratorExp: public ExprNode{
    ExprNode* elt = nullptr;
    Array<Comprehension> generators;
};

// the grammar constrains where yield expressions can occur
struct Await: public ExprNode{
    ExprNode* value;
};

struct Yield: public ExprNode{
    Optional<ExprNode*> value;
};

struct YieldFrom: public ExprNode{
    ExprNode* value = nullptr;
};

// need sequences for compare to distinguish between
// x < 4 < 3 and (x < 4) < 3
struct Compare: public ExprNode{
    ExprNode* left = nullptr;
    Array<CmpOperator> ops;
    Array<ExprNode*> comparators;
};

struct Call: public ExprNode{
    ExprNode* func = nullptr;
    Array<ExprNode*> args;
    Array<Keyword> keywords;
};

struct JoinedStr: public ExprNode{
    Array<ExprNode*> values;
};

struct FormattedValue: public ExprNode{
    ExprNode* value = nullptr;
    Optional<ConversionKind> conversion = ConversionKind::None;
    // defined as ExprNode*
    JoinedStr format_spec;
};

struct Constant: public ExprNode{
    ConstantValue value;
    Optional<String> kind;
};


 // the following expression can appear in assignment context
struct Attribute: public ExprNode{
    ExprNode* value = nullptr;
    Identifier attr;
    ExprContext ctx;
};

struct Subscript: public ExprNode{
    ExprNode* value = nullptr;
    ExprNode* slice = nullptr;
    ExprContext ctx;
};

struct Starred: public ExprNode{
    ExprNode* value = nullptr;
    ExprContext ctx;
};

struct Name: public ExprNode{
    Identifier id;
    ExprContext ctx;

		void print(std::ostream &out) override {
				out << id;
		}
};

struct ListExpr: public ExprNode{
    Array<ExprNode*> elts;
    ExprContext ctx;
};

struct TupleExpr: public ExprNode{
    Array<ExprNode*> elts;
    ExprContext ctx;
};

// can appear only in Subscript
struct Slice: public ExprNode {
    Optional<ExprNode*> lower;
    Optional<ExprNode*> upper;
    Optional<ExprNode*> step;
};

// Modules
// -------
struct Module: public ModNode {
    Array<StmtNode*> body;

    String docstring;
};

struct Interactive: public ModNode {
    Array<StmtNode*> body;
};

struct Expression: public ModNode {
    ExprNode* body = nullptr;
};

struct FunctionType: public ModNode {
    Array<ExprNode*> argtypes;
    ExprNode* returns = nullptr;
};

// Statements
// ----------
struct FunctionDef: public StmtNode {
    Identifier name;
    Arguments args;
    Array<StmtNode*> body;
    Array<ExprNode*> decorator_list;
    Optional<ExprNode*> returns;
    String type_comment;

    String docstring;
    bool async = false;

		void print(std::ostream &out) override {
			out << "def " << name << "(";

			out << ")";

			if (returns.has_value()) {
				out << " -> "; returns.value()->print(out);
			}

			out << ":";

			for(auto& stmt: body) {
				stmt->print(out);
			}
		}
};

struct AsyncFunctionDef: public FunctionDef {
};

struct ClassDef: public StmtNode {
    Identifier name;
    Array<ExprNode*> bases;
    Array<Keyword> keywords;
    Array<StmtNode*> body;
    Array<ExprNode*> decorator_list;

    String docstring;
};

struct Return: public StmtNode {
    Optional<ExprNode*> value;
};

struct Delete: public StmtNode {
    Array<ExprNode*> targets;
};

struct Assign: public StmtNode {
    Array<ExprNode*> targets;
    ExprNode* value = nullptr;
    Optional<String> type_comment;
};

struct AugAssign: public StmtNode{
    ExprNode* target = nullptr;
    BinaryOperator op;
    ExprNode* value = nullptr;
};

// 'simple' indicates that we annotate simple name without parens
struct AnnAssign: public StmtNode {
    ExprNode* target = nullptr;
    ExprNode* annotation = nullptr;
    Optional<ExprNode*> value;
    int simple;
};

// use 'orelse' because else is a keyword in target languages
struct For: public StmtNode {
    ExprNode* target = nullptr;
    ExprNode* iter = nullptr;
    Array<StmtNode*> body;
    Array<StmtNode*> orelse;
    Optional<String> type_comment;

    bool async = false;
};

// Keeping it for consistency with python docs, but useless
struct AsyncFor: public For {

};

struct While: public StmtNode {
    ExprNode* test = nullptr;
    Array<StmtNode*> body;
    Array<StmtNode*> orelse;
};

struct If: public StmtNode {
    ExprNode* test = nullptr;
    Array<StmtNode*> body;
    Array<StmtNode*> orelse;
};

struct With: public StmtNode {
    Array<WithItem> items;
    Array<StmtNode*> body;
    Optional<String> type_comment;

    bool async = false;
};

// Keeping it for consistency with python docs, but useless
struct AsyncWith: public With {

};

struct Raise: public StmtNode {
    Optional<ExprNode*> exc;
    Optional<ExprNode*> cause;
};

struct Try: public StmtNode {
    Array<StmtNode*> body;
    Array<ExceptHandler> handlers;
    Array<StmtNode*> orelse;
    Array<StmtNode*> finalbody;
};

struct Assert: public StmtNode {
    ExprNode* test = nullptr;
    Optional<ExprNode*> msg;
};

struct Import: public StmtNode {
    Array<Alias> names;
};

struct ImportFrom: public StmtNode {
    Optional<Identifier> module;
    Array<Alias> names;
    Optional<int> level;
};

struct Global: public StmtNode {
    Array<Identifier> names;
};

struct Nonlocal: public StmtNode {
    Array<Identifier> names;
};

struct Expr: public StmtNode {
    ExprNode* value = nullptr;
};

struct Pass: public StmtNode {
};

struct Break: public StmtNode {
};

struct Continue: public StmtNode {
};

struct Match: public StmtNode {
    ExprNode* subject;
    Array<MatchCase> cases;
};

//
struct NotImplementedStmt: public StmtNode {};

struct NotImplementedExpr: public ExprNode {};

struct NotAllowedEpxr: public ExprNode {
    String msg;
};

}
#endif
