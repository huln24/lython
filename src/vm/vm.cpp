#include "vm/vm.h"
#include "builtin/operators.h"
#include "utilities/guard.h"
#include "utilities/printing.h"
#include "utilities/strings.h"

namespace lython {

using StmtRet = VMGen::StmtRet;
using ExprRet = VMGen::ExprRet;
using ModRet  = VMGen::ModRet;
using PatRet  = VMGen::PatRet;

ExprRet VMGen::boolop(BoolOp_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::namedexpr(NamedExpr_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::compare(Compare_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::binop(BinOp_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::unaryop(UnaryOp_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::lambda(Lambda_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::ifexp(IfExp_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::dictexpr(DictExpr_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::setexpr(SetExpr_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::listcomp(ListComp_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::generateexpr(GeneratorExp_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::setcomp(SetComp_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::dictcomp(DictComp_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::await(Await_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::yield(Yield_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::yieldfrom(YieldFrom_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::call(Call_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::joinedstr(JoinedStr_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::formattedvalue(FormattedValue_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::constant(Constant_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::attribute(Attribute_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::subscript(Subscript_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::starred(Starred_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::listexpr(ListExpr_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::tupleexpr(TupleExpr_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::slice(Slice_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::dicttype(DictType_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::arraytype(ArrayType_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::arrow(Arrow_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::builtintype(BuiltinType_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::tupletype(TupleType_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::settype(SetType_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::classtype(ClassType_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::comment(Comment_t* n, int depth) { return ExprRet(); }
ExprRet VMGen::name(Name_t* n, int depth) { return ExprRet(); }

// Leaves
StmtRet VMGen::invalidstmt(InvalidStatement_t* n, int depth) {
    kwerror(outlog(), "Invalid statement");
    return StmtRet();
}
StmtRet VMGen::returnstmt(Return_t* n, int depth) {
    add_instruction(n);
    return StmtRet();
}
StmtRet VMGen::deletestmt(Delete_t* n, int depth) {
    add_instruction(n);
    return StmtRet();
}
StmtRet VMGen::assign(Assign_t* n, int depth) {
    add_instruction(n);
    return StmtRet();
}
StmtRet VMGen::augassign(AugAssign_t* n, int depth) {
    add_instruction(n);
    return StmtRet();
}
StmtRet VMGen::annassign(AnnAssign_t* n, int depth) {
    add_instruction(n);
    return StmtRet();
}
StmtRet VMGen::exprstmt(Expr_t* n, int depth) {
    if (Comment* cmt = cast<Comment>(n->value)) {
        return StmtRet();
    }
    add_instruction(n);
    return StmtRet();
}
StmtRet VMGen::pass(Pass_t* n, int depth) { return StmtRet(); }
StmtRet VMGen::breakstmt(Break_t* n, int depth) {
    // jump out
    Jump* jmp = n->new_object<Jump>();
    add_instruction(jmp);
    (*loop_ctx.rbegin()).breakjmp = jmp;

    //
    // add_instruction(n);
    return StmtRet();
}
StmtRet VMGen::continuestmt(Continue_t* n, int depth) {
    Jump* jmp = n->new_object<Jump>();
    add_instruction(jmp);
    (*loop_ctx.rbegin()).continuejmp = jmp;

    // add_instruction(n);
    return StmtRet();
}
StmtRet VMGen::assertstmt(Assert_t* n, int depth) {
    CondJump* jmp = n->new_object<CondJump>();

    // TODO: build the exception AssertionError + assert msg
    Raise* raise = n->new_object<Raise>();

    add_instruction(jmp);

    int raise_idx = instruction_counter();
    add_instruction(raise);

    jmp->condition = n->test;
    jmp->then_jmp  = raise_idx + 1;
    jmp->else_jmp  = raise_idx;

    return StmtRet();
}
StmtRet VMGen::raise(Raise_t* n, int depth) {
    // this is an implicit jump out to an unknown location
    //
    add_instruction(n);
    return StmtRet();
}
StmtRet VMGen::global(Global_t* n, int depth) {
    add_instruction(n);
    return StmtRet();
}
StmtRet VMGen::nonlocal(Nonlocal_t* n, int depth) {
    add_instruction(n);
    return StmtRet();
}

// StmtRet VMGen::condjump(CondJump_t* n, int depth) {
//     return StmtRet();
// }

StmtRet VMGen::import(Import_t* n, int depth) { return StmtRet(); }
StmtRet VMGen::importfrom(ImportFrom_t* n, int depth) { return StmtRet(); }

StmtRet VMGen::inlinestmt(Inline_t* n, int depth) {
    add_body("body", n, n->body, depth);
    return StmtRet();
}
StmtRet VMGen::functiondef(FunctionDef_t* n, int depth) {
    // we cannot add a call instruction because we ignore expression in this VM
    if (n->native) {
        VMNativeFunction* fun = n->new_object<VMNativeFunction>();
        fun->fun              = n->native;
        labels.push_back({n, str(n->name), int(program.size()), depth});
        add_instruction(fun);
    } else {
        add_body(str(n->name), n, n->body, depth);
    }
    return StmtRet();
}
StmtRet VMGen::classdef(ClassDef_t* n, int depth) {
    // Do we need to insert/generate some more functions ?
    //
    //

    // Only functions end up in the final program
    for (auto* stmt: n->body) {
        if (FunctionDef* def = cast<FunctionDef>(stmt)) {
            functiondef(def, depth + 1);
        }
    }

    return StmtRet();
}

StmtRet VMGen::forstmt(For_t* n, int depth) {

    // Target push some value or values to be modified
    // n->target

    // call next on iter and condjump
    // add_body("test", n, n->iter)

    LoopContext& loopctx = loop_ctx.emplace_back();
    KW_DEFERRED([&]() { loop_ctx.pop_back(); });

    int       start = instruction_counter();
    CondJump* jmp   = n->new_object<CondJump>();

    // Get next, if stop iteration jump to
    // we need a break tag and a continue tag
    n->iter;

    add_instruction(jmp);

    jmp->then_jmp = instruction_counter();
    add_body("body", n, n->body, depth);

    jmp->else_jmp = instruction_counter();
    add_body("orelse", n, n->orelse, depth);

    int end = instruction_counter();

    if (Jump* brk = loopctx.breakjmp) {
        // Skip orelse
        brk->destination = end;
    }
    if (Jump* cte = loopctx.continuejmp) {
        // go top of loop
        cte->destination = start;
    }

    return StmtRet();
}

StmtRet VMGen::whilestmt(While_t* n, int depth) {
    LoopContext& loopctx = loop_ctx.emplace_back();
    KW_DEFERRED([&]() { loop_ctx.pop_back(); });

    int       start = instruction_counter();
    CondJump* jmp   = n->new_object<CondJump>();
    jmp->condition  = n->test;
    add_instruction(jmp);

    jmp->then_jmp = instruction_counter();
    add_body("body", n, n->body, depth);

    jmp->else_jmp = instruction_counter();
    add_body("orelse", n, n->orelse, depth);

    int end = instruction_counter();
    if (Jump* brk = loopctx.breakjmp) {
        // Skip orelse
        brk->destination = end;
    }
    if (Jump* cte = loopctx.continuejmp) {
        // go top of loop
        cte->destination = start;
    }

    return StmtRet();
}

StmtRet VMGen::ifstmt(If_t* n, int depth) {
    CondJump* jmp  = n->new_object<CondJump>();
    jmp->condition = n->test;
    add_instruction(jmp);

    jmp->then_jmp = instruction_counter();
    add_body("body", n, n->body, depth);

    jmp->else_jmp = instruction_counter();
    add_body("orelse", n, n->orelse, depth);

    return StmtRet();
}

StmtRet VMGen::with(With_t* n, int depth) {

    // Add enter instructions
    for (WithItem& item: n->items) {
        item.context_expr;
        item.optional_vars;
    }

    // Needs to handle jump to OUTSIDE

    add_body("body", n, n->body, depth);

    // jump to here
    // resume after

    // Add exit instructions
    for (WithItem& item: n->items) {
        item.context_expr;
        item.optional_vars;
    }

    return StmtRet();
}
StmtRet VMGen::trystmt(Try_t* n, int depth) {
    // register the exception handler addresses
    // could `raise Exception` just lookup if "Exception has a registered handler"

    add_body("body", n, n->body, depth);
    // jump to orelse

    // on exception needs to jump to the right address
    for (auto& handler: n->handlers) {
        add_body("except_", n, handler.body, depth);
        // jump to final
    }

    // regular operation
    add_body("orelse", n, n->orelse, depth);

    // Needs to handle jump to OUTSIDE
    add_body("finalbody", n, n->finalbody, depth);

    return StmtRet();
}

StmtRet VMGen::match(Match_t* n, int depth) { return StmtRet(); }

PatRet VMGen::matchvalue(MatchValue_t* n, int depth) { return PatRet(); }
PatRet VMGen::matchsingleton(MatchSingleton_t* n, int depth) { return PatRet(); }
PatRet VMGen::matchsequence(MatchSequence_t* n, int depth) { return PatRet(); }
PatRet VMGen::matchmapping(MatchMapping_t* n, int depth) { return PatRet(); }
PatRet VMGen::matchclass(MatchClass_t* n, int depth) { return PatRet(); }
PatRet VMGen::matchstar(MatchStar_t* n, int depth) { return PatRet(); }
PatRet VMGen::matchas(MatchAs_t* n, int depth) { return PatRet(); }
PatRet VMGen::matchor(MatchOr_t* n, int depth) { return PatRet(); }

ModRet VMGen::module(Module_t* n, int depth) {
    for (auto* stmt: n->body) {
        Super::exec(stmt, depth);
    }
    return ModRet();
};
ModRet VMGen::interactive(Interactive_t* n, int depth) { return ModRet(); }
ModRet VMGen::functiontype(FunctionType_t* n, int depth) { return ModRet(); }
ModRet VMGen::expression(Expression_t* n, int depth) { return ModRet(); }

void exec(int ic, int depth);
//

Value VMExec::execute(Program const& prog, int entry) {
    set_program(&prog);
    execute(entry);
    return Value();
}

Value VMExec::execute(int entry) {
    ic = entry;
    while (true) {
        if (ic >= program->instructions.size() || ic < 0) {
            return getreg(Registers::ReturnValue);
        }

        Instruction const& inst = program->instructions[ic];
        Super::exec(inst.stmt, 0);
        ic += 1;
    }
}

ExprRet VMExec::boolop(BoolOp_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::namedexpr(NamedExpr_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::compare(Compare_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::binop(BinOp_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::unaryop(UnaryOp_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::lambda(Lambda_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::ifexp(IfExp_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::dictexpr(DictExpr_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::setexpr(SetExpr_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::listcomp(ListComp_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::generateexpr(GeneratorExp_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::setcomp(SetComp_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::dictcomp(DictComp_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::await(Await_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::yield(Yield_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::yieldfrom(YieldFrom_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::joinedstr(JoinedStr_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::formattedvalue(FormattedValue_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::constant(Constant_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::attribute(Attribute_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::subscript(Subscript_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::starred(Starred_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::listexpr(ListExpr_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::tupleexpr(TupleExpr_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::slice(Slice_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::dicttype(DictType_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::arraytype(ArrayType_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::arrow(Arrow_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::builtintype(BuiltinType_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::tupletype(TupleType_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::settype(SetType_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::classtype(ClassType_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::comment(Comment_t* n, int depth) { return ExprRet(); }
ExprRet VMExec::name(Name_t* n, int depth) {
    int idx = (n->load_id - n->store_id);
    int size = int(variables.size());
    return variables[size - idx];
}
ExprRet VMExec::call(Call_t* n, int depth) {
    StackTrace& trace = stacktrace.emplace_back();

    // Remove trace when the call finished
    // Remove arguments from addressable variables
    KW_DEFERRED(
        [&](std::size_t old_size) {
            stacktrace.pop_back();
            variables.resize(old_size);
        },
        variables.size());

    // fetch the label to jump to
    int fun_idx = Super::exec(n->func, depth).as<int>();

    // is call native instruction
    Instruction const& inst = program->instructions[fun_idx];
    if (VMNativeFunction* native = cast<VMNativeFunction>(inst.stmt)) {
        Array<Value> args;
        args.reserve(n->args.size());
        for (auto& arg: n->args) {
            args.push_back(Super::exec(arg, depth));
        }

        Function fptr = native->fun;
        return fptr((void*)(this), args);
    } else {
        // check for native call then we can just call it here
        for (auto& arg: n->args) {
            Value arg_val = Super::exec(arg, depth);
            add_value(arg_val);
        }

        // we could modify ic and return
        // but then we need to have a return address
        // here we use an implicit stack which is not good
        // we should use our current call stack
        //
        // BUT in this VM expression are not flatten
        // so we expect call to return an expression
        // in a future version we might flatten the tree further to make it SSA
        // that would simplify execution even further
        int old = ic;
        setreg(Registers::ReturnAddress, -1);

        execute(fun_idx);

        ic = old;
        return getreg(Registers::ReturnValue);
    }
    return Value();
}
// Leaves
StmtRet VMExec::invalidstmt(InvalidStatement_t* n, int depth) {
    kwerror(outlog(), "Invalid statement");
    return StmtRet();
}
StmtRet VMExec::returnstmt(Return_t* n, int depth) {
    if (n->value.has_value()) {
        Value ret_val = Super::exec(n->value.value(), depth);
        setreg(Registers::ReturnValue, ret_val);

        // ic of -1 would stop the program
        // in the case of a call that is what we want
        ic = getreg(Registers::ReturnAddress).as<int>();
    } else {
        setreg(Registers::ReturnValue, Value(_None()));
    }
    return StmtRet();
}
StmtRet VMExec::deletestmt(Delete_t* n, int depth) {
    // delete addressable values
    // they become inusable, access to them should raise
    // an exception
    for (auto* expr: n->targets) {
        // should probably return an address to a value
        Value val = Super::exec(expr, depth);
        int   idx = val.as<int>();
        //
        // free_value(variables[idx]);
        //

        // Make invalid, so use will result in error
        // this only works if addresse directly
        // if the value was part of an object the ref might still
        // think it is valid
        variables[idx] = Value();
    }
    return StmtRet();
}
StmtRet VMExec::assign(Assign_t* n, int depth) {
    Array<ExprNode*>& targets = n->targets;
    Value             val     = Super::exec(n->value, depth);

    kwassert(targets.size() == 1, "");

    switch (targets[0]->kind) {
    case NodeKind::Name: {
        add_value(val);
    }
    // Unpacking
    case NodeKind::TupleExpr: {
        // for ech elem in val
        //  add_value(val)
        //
    }
    }

    return StmtRet();
}
StmtRet VMExec::augassign(AugAssign_t* n, int depth) {
    n->target;
    Value arg = Super::exec(n->value, depth);

    assert(n->target->kind == NodeKind::Name);
    if (Name* name = cast<Name>(n->target)) {
        // lookup
        int idx = 0;

        Array<Value> args(2);
        args[0] = variables[idx];
        args[1] = arg;

        variables[idx] = n->native_operator(this, args);
    }
    return StmtRet();
}
StmtRet VMExec::annassign(AnnAssign_t* n, int depth) {
    ExprNode* target = n->target;
    Value     val;
    if (n->value.has_value()) {
        val = Super::exec(n->value.value(), depth);
    }

    switch (target->kind) {
    case NodeKind::Name: {
        add_value(val);
    }
    // Unpacking
    case NodeKind::TupleExpr: {
        // for ech elem in val
        //  add_value(val)
        //
    }
    }
    return StmtRet();
}
StmtRet VMExec::exprstmt(Expr_t* n, int depth) {
    // have to be called because of side effects
    Super::exec(n->value, depth);
    return StmtRet();
}
StmtRet VMExec::pass(Pass_t* n, int depth) {
    kwassert(false, "should have been ignored");
    return StmtRet();
}
StmtRet VMExec::breakstmt(Break_t* n, int depth) {
    kwassert(false, "should have been converted to jump");
    return StmtRet();
}
StmtRet VMExec::continuestmt(Continue_t* n, int depth) {
    // Jump to loop start
    kwassert(false, "should have been converted to jump");
    return StmtRet();
}
StmtRet VMExec::assertstmt(Assert_t* n, int depth) {
    kwassert(false, "should have been converted to condjump + raise");
    return StmtRet();
}
StmtRet VMExec::raise(Raise_t* n, int depth) {
    // this is an implicit jump out to an unknown location
    //
    return StmtRet();
}
StmtRet VMExec::global(Global_t* n, int depth) { return StmtRet(); }
StmtRet VMExec::nonlocal(Nonlocal_t* n, int depth) { return StmtRet(); }

StmtRet VMExec::nativefunction(VMNativeFunction_t* n, int depth) {
    kwassert(false, "should be handled on the call level");
    return StmtRet();
}


StmtRet VMExec::condjump(CondJump_t* n, int depth) {
    Value val = Super::exec(n->condition, depth);
    ic        = n->then_jmp - 1;
    if (val.as<bool>()) {
        ic = n->else_jmp - 1;
    }
    return StmtRet();
}

StmtRet VMExec::jump(Jump_t* n, int depth) {
    ic = n->destination - 1;
    return StmtRet();
}

StmtRet VMExec::vmstmt(VMStmt* n, int depth) { return Super::exec(n->stmt, depth); }

StmtRet VMExec::import(Import_t* n, int depth) { return StmtRet(); }
StmtRet VMExec::importfrom(ImportFrom_t* n, int depth) { return StmtRet(); }

StmtRet VMExec::inlinestmt(Inline_t* n, int depth) {
    kwassert(false, "should have been converted to instructions");
    return StmtRet();
}
StmtRet VMExec::functiondef(FunctionDef_t* n, int depth) {
    kwassert(false, "should have been converted to NativeCalls & instructions");
    return StmtRet();
}
StmtRet VMExec::classdef(ClassDef_t* n, int depth) {
    kwassert(false, "should have been converted to calls");
    return StmtRet();
}

StmtRet VMExec::forstmt(For_t* n, int depth) {
    kwassert(false, "should have been converted to jump");
    return StmtRet();
}

StmtRet VMExec::whilestmt(While_t* n, int depth) {
    kwassert(false, "should have been converted to jump");
    return StmtRet();
}

StmtRet VMExec::ifstmt(If_t* n, int depth) {
    kwassert(false, "should have been converted to jump");
    return StmtRet();
}

StmtRet VMExec::with(With_t* n, int depth) {
    kwassert(false, "should have been converted to calls & jumps");
    return StmtRet();
}
StmtRet VMExec::trystmt(Try_t* n, int depth) {
    kwassert(false, "should have been converted to jumps");
    return StmtRet();
}

StmtRet VMExec::exported(Exported_t* n, int depth) { return StmtRet(); }

StmtRet VMExec::placeholder(Placeholder_t* n, int depth) { return StmtRet(); }

StmtRet VMExec::match(Match_t* n, int depth) {
    kwassert(false, "should have been converted to jumps");
    return StmtRet();
}

PatRet VMExec::matchvalue(MatchValue_t* n, int depth) { return PatRet(); }
PatRet VMExec::matchsingleton(MatchSingleton_t* n, int depth) { return PatRet(); }
PatRet VMExec::matchsequence(MatchSequence_t* n, int depth) { return PatRet(); }
PatRet VMExec::matchmapping(MatchMapping_t* n, int depth) { return PatRet(); }
PatRet VMExec::matchclass(MatchClass_t* n, int depth) { return PatRet(); }
PatRet VMExec::matchstar(MatchStar_t* n, int depth) { return PatRet(); }
PatRet VMExec::matchas(MatchAs_t* n, int depth) { return PatRet(); }
PatRet VMExec::matchor(MatchOr_t* n, int depth) { return PatRet(); }

ModRet VMExec::module(Module_t* n, int depth) {
    for (auto* stmt: n->body) {
        Super::exec(stmt, depth);
    }
    return ModRet();
};
ModRet VMExec::interactive(Interactive_t* n, int depth) { return ModRet(); }
ModRet VMExec::functiontype(FunctionType_t* n, int depth) { return ModRet(); }
ModRet VMExec::expression(Expression_t* n, int depth) { return ModRet(); }

}  // namespace lython
