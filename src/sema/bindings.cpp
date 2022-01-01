#include "ast/magic.h"
#include "ast/ops.h"
#include "sema/bindings.h"
#include "utilities/strings.h"

namespace lython {

void Bindings::dump(std::ostream &out) const {
    auto big   = String(40, '-');
    auto small = String(20, '-');
    auto sep   = fmt::format("{:>40}-+-{:>20}-+-{}", big, small, small);

    out << sep << '\n';
    out << fmt::format("{:40} | {:20} | {}", "name", "type", "value") << "\n";
    out << sep << '\n';
    for (auto &e: bindings) {
        print(out, e);
    }
    out << sep << '\n';
}

inline std::ostream &print(std::ostream &out, BindingEntry const &entry) {
    String n = str(entry.name);
    String v = str(entry.value);
    String t = str(entry.type);

    auto frags = split('\n', v);

    out << fmt::format("{:>40} | {:>20} | {}", n, t, frags[0]) << '\n';
    for (int i = 1; i < frags.size(); i++) {
        if (strip(frags[i]) == "") {
            continue;
        }
        out << fmt::format("{:>40} | {:>20} | {}", "", "", frags[i]) << '\n';
    }
    return out;
}

} // namespace lython