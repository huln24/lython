#include "nodes.h"
#include "utilities/printing.h"


namespace lython {

ExprNode* None();
ExprNode* True();
ExprNode* False();

std::ostream& operator<<(std::ostream& out, UnaryOperator const& v) ;
std::ostream& operator<<(std::ostream& out, BinaryOperator const& v) ;
std::ostream& operator<<(std::ostream& out, BoolOperator const& v) ;
std::ostream& operator<<(std::ostream& out, CmpOperator const& v) ;

// --------------------------------------------------------------------
// to-string

String str(NodeKind k) {
#define CASEGEN(name)    \
    case NodeKind::name: \
        return #name;

#define X(name, _)     CASEGEN(name)
#define SECTION(name)  CASEGEN(name)
#define EXPR(name, _)  CASEGEN(name)
#define STMT(name, _)  CASEGEN(name)
#define MOD(name, _)   CASEGEN(name)
#define MATCH(name, _) CASEGEN(name)
#define VM(name, _) CASEGEN(name)

    switch (k) {
        NODEKIND_ENUM(X, SECTION, EXPR, STMT, MOD, MATCH, VM)

    default: break;
    }

#undef X
#undef SECTION
#undef EXPR
#undef STMT
#undef MOD
#undef MATCH
#undef VM
    return "<invalid>";
}

std::ostream& operator<<(std::ostream& out, BoolOperator const& v) {
    switch (v) {
#define OP(name, kw, _)        \
    case BoolOperator::name: { \
        out << #kw;            \
        return out;                \
    }
        BOOL_OPERATORS(OP)

    default: break;
#undef OP
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, BinaryOperator const& v) {
    switch (v) {
#define OP(name, kw, _)          \
    case BinaryOperator::name: { \
        out << #name;            \
        return out;                  \
    }
        BINARY_OPERATORS(OP)

    default: break;
#undef OP
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, UnaryOperator const& v) {
    switch (v) {
#define OP(name, kw, _)         \
    case UnaryOperator::name: { \
        out << #name;           \
        return out;                 \
    }
        UNARY_OPERATORS(OP)

#undef OP
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, CmpOperator const& v) {
    switch (v) {
#define OP(name, kw, _)       \
    case CmpOperator::name: { \
        out << #name;         \
        return out;               \
    }
        COMP_OPERATORS(OP)

#undef OP
    }
    return out;
}

void ClassDef::Attr::dump(std::ostream& out) {
    out << name << ": " << str(type) << " = " << str(stmt);
}

StringRef operator_magic_name(BinaryOperator const& v, bool reverse) {

#define OP(name, kw, magic)                                \
    static StringRef m_##name  = String("__" #magic "__"); \
    static StringRef mr_##name = String("__r" #magic "__");

    BINARY_OPERATORS(OP)

#undef OP

    switch (v) {
#define OP(name, kw, _)                        \
    case BinaryOperator::name: {               \
        return reverse ? mr_##name : m_##name; \
    }
        BINARY_OPERATORS(OP)

#undef OP
    }

    return StringRef("");
}

StringRef operator_magic_name(BoolOperator const& v, bool reverse) {

#define OP(name, kw, magic)                                \
    static StringRef m_##name  = String("__" #magic "__"); \
    static StringRef mr_##name = String("__r" #magic "__");

    BOOL_OPERATORS(OP)

#undef OP

    switch (v) {
#define OP(name, kw, _)                        \
    case BoolOperator::name: {                 \
        return reverse ? mr_##name : m_##name; \
    }
        BOOL_OPERATORS(OP)

    default: break;
#undef OP
    }

    return StringRef("");
}

StringRef operator_magic_name(UnaryOperator const& v, bool reverse) {

#define OP(name, kw, magic)                                \
    static StringRef m_##name  = String("__" #magic "__"); \
    static StringRef mr_##name = String("__r" #magic "__");

    UNARY_OPERATORS(OP)

#undef OP

    switch (v) {
#define OP(name, kw, _)                        \
    case UnaryOperator::name: {                \
        return reverse ? mr_##name : m_##name; \
    }
        UNARY_OPERATORS(OP)

#undef OP
    }

    return StringRef("");
}

StringRef operator_magic_name(CmpOperator const& v, bool reverse) {

#define OP(name, kw, magic)                                \
    static StringRef m_##name  = String("__" #magic "__"); \
    static StringRef mr_##name = String("__r" #magic "__");

    COMP_OPERATORS(OP)

#undef OP

    switch (v) {
#define OP(name, kw, _)                        \
    case CmpOperator::name: {                  \
        return reverse ? mr_##name : m_##name; \
    }
        COMP_OPERATORS(OP)

#undef OP
    }

    return StringRef("");
}

// FIXME: this is a hack
// to avoid cicle we should just make a reference to the type that cycles
bool Arrow::add_arg_type(ExprNode* arg_type) {
    if (arg_type != this) {
        args.push_back(arg_type);

        if (has_circle(this)) {
            args.pop_back();
            return false;
        }

        return true;
    }
    kwwarn(outlog(), "trying to adding self to an arrow argument");
    return false;
}

bool Arrow::set_arg_type(int i, ExprNode* arg_type) {

    if (arg_type != this) {
        ExprNode* old = args[i];
        args[i]       = arg_type;

        if (has_circle(this)) {
            args[i] = old;
            return false;
        }

        return true;
    }

    kwwarn(outlog(), "trying to assing self to an arrow argument");
    return false;
}

// ------------------------------------------
}  // namespace lython
