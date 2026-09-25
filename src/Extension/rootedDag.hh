#ifndef MAUDE_SE_ROOTED_DAG_HH
#define MAUDE_SE_ROOTED_DAG_HH

#include "dagNode.hh"
#include "importModule.hh"
#include "rootContainer.hh"
#include "symbol.hh"

// A shared_ptr to this object keeps a DAG visible to Maude's collector.
// A shared_ptr<DagNode> alone would not register a GC root.
class RootedDag : private RootContainer
{
public:
    explicit RootedDag(DagNode *dag)
        : dag(dag), module(dag ? dynamic_cast<ImportModule *>(dag->symbol()->getModule()) : nullptr)
    {
        link();
        if (module) module->protect();
    }
    ~RootedDag() override
    {
        unlink();
        if (module) module->unprotect();
    }
    RootedDag(const RootedDag &) = delete;
    RootedDag &operator=(const RootedDag &) = delete;
    DagNode *get() const { return dag; }

private:
    void markReachableNodes() override
    {
        if (dag) dag->mark();
    }

    DagNode *dag;
    ImportModule *module;
};

#endif
