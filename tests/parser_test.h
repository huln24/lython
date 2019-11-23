#include "samples.h"

#include <sstream>

#include "lexer/buffer.h"
#include "parser/parser.h"

using namespace lython;


inline String parse_it(String code){
    StringBuffer reader(code);
    Module module;

    Parser par(reader, &module);

    StringStream ss;

    ST::Expr expr = nullptr;
    do {
        expr = par.parse_one(module);

        if (expr){
            expr->print(ss) << '\n';
        }

    } while(expr != nullptr);
    return ss.str();
}

String strip(String const& v){
    int i = int(v.size()) - 1;

    while (i > 0 && v[size_t(i)] == '\n'){
        i -= 1;
    }

    return String(v.begin(), v.begin() + i + 1);
}


#define TEST_PARSING(code)\
    SECTION(#code){\
        REQUIRE(strip(parse_it(code())) == strip(code()));\
    }

TEST_CASE("Parser"){
    CODE_SAMPLES(TEST_PARSING)
}
