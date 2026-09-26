#include "macros.hh"
#include "vector.hh"
#include "interface.hh"
#include "core.hh"
#include "variable.hh"
#include "mixfix.hh"
#include "SMT.hh"
#include "symbol.hh"
#include "term.hh"

#if defined(MAUDE_SE_PLUGIN_Z3)
#include "z3.hh"
using PluginFactory = Z3SmtManagerFactory;
#define PLUGIN_SOLVER "z3"
#elif defined(MAUDE_SE_PLUGIN_YICES)
#include "yices2.hh"
using PluginFactory = YicesSmtManagerFactory;
#define PLUGIN_SOLVER "yices"
#elif defined(MAUDE_SE_PLUGIN_CVC5)
#include "cvc5.hh"
using PluginFactory = Cvc5SmtManagerFactory;
#define PLUGIN_SOLVER "cvc5"
#else
#error "Select exactly one native SMT plugin"
#endif

extern "C" {
__attribute__((visibility("default"))) int maude_se_plugin_abi() { return 1; }
__attribute__((visibility("default"))) const char *maude_se_plugin_solver()
{
    return PLUGIN_SOLVER;
}
__attribute__((visibility("default"))) SmtManagerFactory *maude_se_plugin_factory()
{
    static PluginFactory factory;
    return &factory;
}
}
