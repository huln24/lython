#include "sema/builtin.h"

namespace lython {

BuiltinType make_type(String const &name) {
    auto expr = BuiltinType();
    expr.name = name;
    return expr;
}

#define TYPE(name)                                  \
    TypeExpr *name##_t() {                          \
        static BuiltinType type = make_type(#name); \
        return &type;                               \
    }

BUILTIN_TYPES(TYPE)

#undef TYPE

ExprNode *none() {
    static Constant constant(ConstantValue::none());
    return &constant;
}

ExprNode *True() {
    static Constant constant(true);
    return &constant;
}

ExprNode *False() {
    static Constant constant(false);
    return &constant;
}

} // namespace lython