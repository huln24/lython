#include "parser/parser.h"

namespace lython {

NEW_EXCEPTION(PrimaryExpression);

// Math Nodes for ReverPolish parsing
enum class MathKind {
    Operator,
    Value,
    Function,
    VarRef,
    None
};

struct MathNode {
    MathKind kind;
    int arg_count = 1;
    Expression ref;
    String name = "";
};

Expression Parser::parse_primary(Module& m, std::size_t depth){
    TRACE_START();
    Token tok = token();

    switch(tok.type()){
    // values
    case tok_int:{
        auto r = Expression::make<AST::Value>(tok.as_integer(), m.find("Int"));
        next_token();
        return r;
    }

    case tok_string:{
        auto r = Expression::make<AST::Value>(tok.identifier(), m.find("String"));
        next_token();
        return r;
    }

    case tok_float:{
        auto r = Expression::make<AST::Value>(tok.as_float(), m.find("Float"));
        next_token();
        return r;
    }

    case '(': {
        next_token();
        auto r = parse_expression(m, depth + 1);
        EAT(')');
        return r;
    }
    // case indent ?
    // => compound statement

    // Unary Operator
    case tok_operator:{
        auto expr = parse_expression(m, depth + 1);
        return Expression::make<AST::UnaryOperator>(tok.operator_name(), expr);
    }

    // Function or Variable
    case tok_identifier:{
        auto expr = m.find(tok.identifier());
        next_token();

        // Function
        if (token().type() == '('){
            return parse_function_call(m, expr, depth + 1);
        }

        // Variable
        return expr;
    }

    // ---
    default:
        throw PrimaryExpression(
            "Expected {} got {}", to_string(tok.type()));
    }
}


Expression Parser::parse_expression(Module& m, std::size_t depth){
    auto lhs = parse_primary(m, depth);

    if (token().type() == tok_operator){
        return parse_expression_1(m, lhs, 0, depth);
    }

    return lhs;
}

Expression Parser::parse_expression_1(Module& m, Expression lhs, int precedence, std::size_t depth){
    TRACE_START();
    Token tok = token();
    EXPECT(tok_operator, "Expect an operator");

    auto get_op_config = [m](String const& op) -> OpConfig const& {
        return default_precedence()[op];
    };

    // if not a binary op precedence is -1
    while (true){
        tok = token();

        String op;
        if (tok.type() == tok_operator){
            op = tok.operator_name();
        }

        OpConfig const& op_conf = get_op_config(op);
        if (op_conf.precedence < precedence){
            return lhs;
        }

        tok = next_token();
        auto rhs = parse_primary(m, depth + 1);

        if (!rhs){
            return Expression();
        }

        tok = token();
        String op2;
        if (tok.type() == tok_operator){
            op2 = tok.operator_name();
        }

        OpConfig const& op_conf2 = get_op_config(op2);
        if (op_conf.precedence < op_conf2.precedence){
            rhs = parse_expression_1(m, rhs, op_conf.precedence + 1, depth + 1);
            if (!rhs){
                return Expression();
            }
        }

        lhs = Expression::make<AST::BinaryOperator>(lhs, rhs, get_string(op));
    }
}

void debug_dump(std::ostream& out, Stack<AST::MathNode> const& output_stack){
    auto riter = output_stack.rbegin();

    while (riter != output_stack.rend()) {
        out << (*riter).name << " ";
        ++riter;
    }
    out << "\n";

    auto iter = output_stack.begin();
    while (iter != output_stack.end()) {
        auto op = *iter;
        out << (*iter).name << " ";
        ++iter;
    }
    out << "\n";
}

// return function call
Expression Parser::parse_function_call(Module& m, Expression function, std::size_t depth) {
    TRACE_START();
    auto expr = Expression::make<AST::Call>();
    auto fun = expr.ref<AST::Call>();

    fun->function = function;
    EXPECT('(', "`(` was expected");
    EAT('(');

    while (token().type() != ')') {
        fun->arguments.push_back(parse_expression(m, depth + 1));

        if (token().type() == ',') {
            next_token();
        }
    }

    EXPECT(')', "`)` was expected");
    EAT(')');
    return expr;
}

} // namespace lython
