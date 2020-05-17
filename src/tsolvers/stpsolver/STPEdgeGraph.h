#ifndef OPENSMT_STPEDGEGRAPH_H
#define OPENSMT_STPEDGEGRAPH_H

#include "STPStore.h"
#include "STPMapper.h"

class STPEdgeGraph {

    STPStore & store;

    STPMapper & mapper;

    uint32_t addedCount;

    std::vector<EdgeRef> addedEdges;    // FIXME: Probably should be stored somewhere else

    using AdjList = std::vector<EdgeRef>;
    std::vector<AdjList> incoming, outgoing;

    size_t dfsSearch(VertexRef init, std::vector<bool> &visited, std::vector<opensmt::Number> &length, bool forward);

public:
    explicit STPEdgeGraph(STPStore &store, STPMapper &mapper) : store(store), mapper(mapper), addedCount(0) {}
    bool isTrue(EdgeRef e) const;
    uint32_t getAddedCount() const { return addedCount; }
    void setTrue(EdgeRef e, bool consequence);
    void findConsequences(EdgeRef e);
    void removeAfter(uint32_t point);
};


#endif //OPENSMT_STPEDGEGRAPH_H