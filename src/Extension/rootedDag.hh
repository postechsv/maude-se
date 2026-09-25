#ifndef MAUDE_SE_ROOTED_DAG_HH
#define MAUDE_SE_ROOTED_DAG_HH

#include "dagNode.hh"
#include "importModule.hh"
#include "rootContainer.hh"
#include "symbol.hh"
#include <memory>
#include <set>
#include <vector>

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

// A value-type handle for DAGs that cross an API boundary. Copying the
// handle shares the GC root; it never takes ownership of the DagNode itself.
class DagHandle
{
public:
    DagHandle() = default;
    explicit DagHandle(DagNode *dag)
        : root(dag ? std::make_shared<RootedDag>(dag) : nullptr) {}

    DagNode *get() const { return root ? root->get() : nullptr; }
    explicit operator bool() const { return get() != nullptr; }

private:
    std::shared_ptr<RootedDag> root;
};

// One GC root for all temporary DAGs created during a conversion or while
// assembling a Maude result. The caller must add each new DAG before doing
// any further work that could reach a Maude GC safe point.
class DagRootFrame : private RootContainer
{
public:
    DagRootFrame() { link(); }
    ~DagRootFrame() override
    {
        unlink();
        for (ImportModule *module : modules) module->unprotect();
    }
    DagRootFrame(const DagRootFrame &) = delete;
    DagRootFrame &operator=(const DagRootFrame &) = delete;

    DagNode *keep(DagNode *dag)
    {
        if (dag)
        {
            if (auto *module = dynamic_cast<ImportModule *>(dag->symbol()->getModule()))
                if (modules.insert(module).second) module->protect();
            dags.push_back(dag);
        }
        return dag;
    }

private:
    void markReachableNodes() override
    {
        for (DagNode *dag : dags) dag->mark();
    }

    std::vector<DagNode *> dags;
    std::set<ImportModule *> modules;
};

class ActiveDagRootFrame
{
public:
    ActiveDagRootFrame(DagRootFrame *&slot, DagRootFrame &frame)
        : slot(slot), previous(slot) { slot = &frame; }
    ~ActiveDagRootFrame() { slot = previous; }
    ActiveDagRootFrame(const ActiveDagRootFrame &) = delete;
    ActiveDagRootFrame &operator=(const ActiveDagRootFrame &) = delete;

private:
    DagRootFrame *&slot;
    DagRootFrame *previous;
};

#endif
