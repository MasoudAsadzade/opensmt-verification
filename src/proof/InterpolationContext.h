/*
 *  Copyright (c) 2020-2022, Martin Blicha <martin.blicha@gmail.com>
 *
 *  SPDX-License-Identifier: MIT
 *
 */

#ifndef OPENSMT_INTERPOLATIONCONTEXT_H
#define OPENSMT_INTERPOLATIONCONTEXT_H

#include <memory>

#include "Theory.h"

// forward declaration
class Proof;
class ProofGraph;

class InterpolationContext {
    std::unique_ptr<ProofGraph> proof_graph;
public:
    InterpolationContext(SMTConfig & c, Theory & th, TermMapper & termMapper, Proof const & t,
                         PartitionManager& pmanager , int n = -1 );
    ~InterpolationContext();

    void printProofDotty();

    // Create interpolants with each A consisting of the specified partitions
    void getInterpolants(const std::vector<vec<int> > & partitions, vec<PTRef> & interpolants);

    void setColoringSuggestions(vec<std::map<PTRef, icolor_t> *> * mp);

    void getSingleInterpolant(vec<PTRef> & interpolants);

    void getSingleInterpolant(vec<PTRef> & interpolants, const ipartitions_t & A_mask);

    void getSingleInterpolant(std::vector<PTRef>& interpolants, const ipartitions_t& A_mask);

    bool getPathInterpolants(vec<PTRef> & interpolants, const std::vector<ipartitions_t> & A_masks);

private:
    void reduceProofGraph();

};


#endif //OPENSMT_INTERPOLATIONCONTEXT_H
