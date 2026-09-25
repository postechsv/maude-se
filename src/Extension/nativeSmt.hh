#ifndef NATIVE_SMT_HH
#define NATIVE_SMT_HH

#include "freeDagNode.hh"
#include "SMT_Info.hh"
#include "rootedDag.hh"
#include <optional>
#include <unordered_map>
#include <vector>

/*
 * Native Abstract class for SMT Converter
 */
template < typename T, typename U >
class NativeSmtConverter
{
protected:
    /*
     */
    const SMT_Info& smtInfo;
    typedef std::map<DagNode*, T> SmtManagerVariableMap;
    typedef std::map<T, DagNode*, U> ReverseSmtManagerVariableMap;

protected:

    SmtManagerVariableMap smtManagerVariableMap;

private:
    struct ConversionEntry
    {
        DagHandle dag;
        T term;
    };
    std::unordered_map<size_t, std::vector<ConversionEntry>> conversionCache;
    size_t conversionCacheSize = 0;

protected:
    void clearConversionCache()
    {
        conversionCache.clear();
        conversionCacheSize = 0;
    }

    std::optional<T> cachedConversion(DagNode *dag) const
    {
        auto bucket = conversionCache.find(dag->getHashValue());
        if (bucket != conversionCache.end())
            for (const auto &entry : bucket->second)
                if (dag == entry.dag.get() || dag->equal(entry.dag.get()))
                    return entry.term;
        return std::nullopt;
    }

    void rememberConversion(DagNode *dag, const T &term)
    {
        // Each key retains its Maude GC root. Bound both roots and solver terms.
        if (conversionCacheSize >= 4096) clearConversionCache();
        conversionCache[dag->getHashValue()].push_back({DagHandle(dag), term});
        ++conversionCacheSize;
    }

public:

    NativeSmtConverter(const SMT_Info& smtInfo):
        smtInfo(smtInfo) {}

    virtual ~NativeSmtConverter(){
        clearConversionCache();
        smtManagerVariableMap.clear();
    }

protected:
    
    /*
     * makeVariable : Generates an SMT variable.
     */
    virtual T makeVariable(DagNode* dag) = 0;

protected:
    ReverseSmtManagerVariableMap*
    generateReverseVariableMap(){
        ReverseSmtManagerVariableMap* rsv = new ReverseSmtManagerVariableMap();
        for (auto it = smtManagerVariableMap.begin(); it != smtManagerVariableMap.end(); it++) {
            (*rsv)[it->second] = it->first;
        }
        return rsv;
    }
};


#endif
