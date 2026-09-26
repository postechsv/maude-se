#include "extGlobal.hh"
#include <dlfcn.h>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

SmtManagerFactory* smtManagerFactory = nullptr;
char* smtSolver = nullptr;

void setSmtSolver(char* solver){
    smtSolver = solver;
}

void setSmtManagerFactory(SmtManagerFactory* fac){
    smtManagerFactory = fac;    
}

namespace {
std::string pluginError;
std::vector<void *> pluginHandles;
std::map<std::string, std::pair<std::string, SmtManagerFactory *>> loadedPlugins;
}

const char* nativeSmtPluginError() { return pluginError.c_str(); }

bool loadNativeSmtPlugin(const char* path, const char* solver){
    pluginError.clear();
    if (!path || !solver) {
        pluginError = "plugin path and solver are required";
        return false;
    }
    auto cached = loadedPlugins.find(path);
    if (cached != loadedPlugins.end()) {
        if (cached->second.first != solver) {
            pluginError = "incompatible native SMT plugin";
            return false;
        }
        setSmtManagerFactory(cached->second.second);
        return true;
    }
    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        pluginError = dlerror();
        return false;
    }
    using Abi = int (*)();
    using Name = const char* (*)();
    using Factory = SmtManagerFactory* (*)();
    auto abi = reinterpret_cast<Abi>(dlsym(handle, "maude_se_plugin_abi"));
    auto name = reinterpret_cast<Name>(dlsym(handle, "maude_se_plugin_solver"));
    auto factory = reinterpret_cast<Factory>(dlsym(handle, "maude_se_plugin_factory"));
    if (!abi || !name || !factory || abi() != 1 ||
        !name() || std::strcmp(name(), solver) != 0) {
        pluginError = "incompatible native SMT plugin";
        dlclose(handle);
        return false;
    }
    SmtManagerFactory *candidate = factory();
    if (!candidate) {
        pluginError = "native SMT plugin returned no factory";
        dlclose(handle);
        return false;
    }
    setSmtManagerFactory(candidate);
    // Search objects can outlive a backend switch, so never unload a live plugin.
    pluginHandles.push_back(handle);
    loadedPlugins.emplace(path, std::make_pair(std::string(solver), candidate));
    return true;
}
