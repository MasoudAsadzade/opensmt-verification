/*
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#include "ScatterSplitter.h"
#include "Proof.h"
#include "SystemQueries.h"
#include "ReportUtils.h"
#include "OsmtInternalException.h"

namespace opensmt
{
    extern bool stop;
}

ScatterSplitter::ScatterSplitter(SMTConfig & c, THandler & t)
    : Splitter              (c, decisions)
    , SimpSMTSolver         (c, t)
{}

bool ScatterSplitter::branchLitRandom() {
    return ((not splitContext.isInSplittingCycle() and drand(random_seed) < random_var_freq) or
            (splitContext.isInSplittingCycle() and splitContext.preferRandom()))
           and not order_heap.empty();
}

Var ScatterSplitter::doActivityDecision() {
    vec<int> discarded;
    Var next = var_Undef;
    while (next == var_Undef || value(next) != l_Undef || !decision[next]) {
        if (order_heap.empty()) {
            if (discarded.size() > 0) {
                assert(splitContext.isInSplittingCycle());
                assert([&]() {
                    for (Var v : discarded) {
                        if (not isAssumptionVar(v)) {
                            throw OsmtInternalException(";assert split partition: discarded var was not found in assumption vars");
                        }
                    }
                    return true;
                }());

                // All available literals are assumption literals.  Instance is satisfiable
            } else {
                next = var_Undef;
            }
            break;
        } else {
            next = order_heap.removeMin();
            if (splitContext.isInSplittingCycle() && next != var_Undef) {
                if (isAssumptionVar(next)) {
                    discarded.push(next);
                    next = var_Undef;
                    // A hack!: Not branch on lengthy variables (more than 5000), basically not allowing split partition with too many nested loop
                } else if (this->theory_handler.getLogic().dumpWithLets(theory_handler.varToTerm(next)).length() > 5000) {
                    discarded.push(next);
                    next = var_Undef;
                } else if (splitContext.preferTerm() && !theory_handler.isDeclared(next)) {
                    discarded.push(next);
                    next = var_Undef;
                } else if (splitContext.preferTermNotEq() && (!theory_handler.isDeclared(next) or theory_handler.getLogic().isEquality(theory_handler.varToTerm(next)))) {
                    discarded.push(next);
                    next = var_Undef;
                } else if (splitContext.preferFormula() && theory_handler.isDeclared(next)) {
                    discarded.push(next);
                    next = var_Undef;
                }
                else if (splitContext.preferNotEq() && theory_handler.getLogic().isEquality(theory_handler.varToTerm(next))) {
                    discarded.push(next);
                    next = var_Undef;
                } else if (splitContext.preferEq() && not theory_handler.getLogic().isEquality(theory_handler.varToTerm(next))) {
                    discarded.push(next);
                    next = var_Undef;
                }
            }
        }
    }
    for (Var v : discarded)
        order_heap.insert(v);

    return next;
}

bool ScatterSplitter::okContinue() const {
    if (!CoreSMTSolver::okContinue()) {
        return false;
    } else if (conflicts % 1000 == 0 and splitContext.resourceLimitReached(decisions)) {
        opensmt::stop = true;
        return false;
    } else if (static_cast<int>(splitContext.getCurrentSplitCount()) == splitContext.splitTargetNumber() - 1) {
        return false;
    }
    return true;
}

void ScatterSplitter::notifyEnd() {
    if (config.sat_split_type() == spt_scatter) {
        auto[data, result] = createSplitAndBlockAssumptions();
        splitContext.insertSplitData(std::move(data));
        assert(result == l_False);
        (void) result;
    }
    opensmt::stop = true;
}

lbool ScatterSplitter::solve_() {
    if (config.sat_split_type() == spt_scatter) {
        splitContext.reset(decisions);
        splitContext.enterInitCycle(decisions);
    }
    return CoreSMTSolver::solve_();
}

lbool ScatterSplitter::zeroLevelConflictHandler() {
    if (splitContext.hasCurrentSplits()) {
        opensmt::stop = true;
        return l_Undef;
    } else {
        return CoreSMTSolver::zeroLevelConflictHandler();
    }
}

bool ScatterSplitter::scatterLevel() {
    int d;
    int currentSplitNum = static_cast<int>(splitContext.getCurrentSplitCount());
    int targetSplitNum = splitContext.splitTargetNumber();
    int splitsToDo = targetSplitNum - currentSplitNum;
    assert(splitsToDo > 0);
    // Current scattered instance number i = splits.size() + 1
    float r = 1/(float)(splitsToDo);
    for (int i = 1; ; i++) {
        // 2 << i == 2^(i+1)
        if ((2 << (i-1) <= splitsToDo) && (2 << i >= splitsToDo)) {
            // r-1(2^i) < 0 and we want absolute
            d = -(r-1/(float)(2<<(i-1))) > r-1/(float)(2<<i) ? i+1 : i;
            break;
        }
    }
    return d == decisionLevel() - assumptions.size();
}

opensmt::pair<SplitData,lbool> ScatterSplitter::createSplitAndBlockAssumptions() {
    assert(splitContext.getCurrentSplitCount() == static_cast<int>(split_assumptions.size()));
    SplitData splitData;
    vec<Lit> constraints_negated;
    vec<Lit> split_assumption;
    // Add the literals on the decision levels
    for (int i = assumptions.size(); i < decisionLevel(); i++) {
        Lit l = trail[trail_lim[i]];
        splitData.addConstraint<vec<Lit>>({l});
        // Remember this literal in the split assumptions vector of the
        // SAT solver
        split_assumption.push(l);
        // This will be added to the SAT formula to exclude the search
        // space
        constraints_negated.push(~l);
    }
    split_assumptions.emplace_back(std::move(split_assumption));
    for (size_t i = 0; i < split_assumptions.size()-1; i++) {
        vec<Lit> tmp;
        for (auto tr : split_assumptions[i]) {
            tmp.push(~tr);
        }
        splitData.addConstraint(tmp);
    }

    cancelUntil(0);
    lbool res = excludeAssumptions(constraints_negated) ? l_Undef : l_False;

    return {std::move(splitData), res};
}

bool ScatterSplitter::excludeAssumptions(vec<Lit> const & neg_constrs) {
    addOriginalClause(neg_constrs);
    simplify();
    return ok;
}

CoreSMTSolver::ConsistencyAction ScatterSplitter::notifyConsistency() {
    if (not splitContext.isInSplittingCycle() and splitContext.shouldEnterSplittingCycle(decisions)) {
        splitContext.enterSplittingCycle();
        return ConsistencyAction::BacktrackToZero;
    }
    if (splitContext.isInSplittingCycle() and scatterLevel()) {
        auto [data, result] = createSplitAndBlockAssumptions();
        splitContext.insertSplitData(std::move(data));
        if (result == l_False) { // Rest is unsat
            opensmt::stop = true;
            return ConsistencyAction::ReturnUndef;
        } else {
            splitContext.enterTuningCycle(decisions);
            return ConsistencyAction::SkipToSearchBegin;
        }
    }
    return ConsistencyAction::NoOp;
}
