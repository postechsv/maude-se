#ifndef NATIVE_SMT_HH
#define NATIVE_SMT_HH

#include "freeDagNode.hh"
#include "SMT_Info.hh"

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

public:

    NativeSmtConverter(const SMT_Info& smtInfo):
        smtInfo(smtInfo) {}

    virtual ~NativeSmtConverter(){
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
