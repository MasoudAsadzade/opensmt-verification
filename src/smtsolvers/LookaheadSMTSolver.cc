//
// Created by prova on 07.02.19.
//

#include "LookaheadSMTSolver.h"

LookaheadSMTSolver::LookaheadSMTSolver(SMTConfig& c, THandler& thandler)
	: SimpSMTSolver(c, thandler)
    , idx(0)
	, score(c.lookahead_score_deep() ? (LookaheadScore*)(new LookaheadScoreDeep(assigns, c)) : (LookaheadScore*)(new LookaheadScoreClassic(assigns, c)))
{}

Var LookaheadSMTSolver::newVar(bool dvar) {
    Var v = SimpSMTSolver::newVar(dvar);
    score->newVar();
    return v;
}

lbool LookaheadSMTSolver::solve_() {
    declareVarsToTheories();

    double nof_conflicts = restart_first;

    LALoopRes res = LALoopRes::unknown;

    while (res == LALoopRes::unknown || res == LALoopRes::restart) {
        //cerr << "; Doing lookahead for " << nof_conflicts << " conflicts\n";
        ConflQuota conflict_quota;
        //if (config.lookahead_restarts()) {
        //    conflict_quota = ConflQuota((int)nof_conflicts);
        //}
        res = solveLookahead();

        nof_conflicts = restartNextLimit(nof_conflicts);
    }

    if (res == LALoopRes::sat) {
        model.growTo(nVars());
        for (unsigned int i = 0; i < dec_vars; i++) {
            Var p = var(trail[i]);
            model[p] = value(p);
        }
    }
    switch (res) {
        case LALoopRes::unknown_final:
            return l_Undef;
        case LALoopRes::sat:
            return l_True;
        case LALoopRes::unsat: {
            ok = false;
            return l_False;
        }
        default:
            assert(false);
            return l_Undef;
    }
}

//
// Function for making a propagation.
// max_confl is a bound on the number of conflicts the wrapper is allowed to do
// Returns l_Undef if the bound on conflicts is reached.
// Returns l_False if the there was a conflict.
// Returns l_True if there was no conflict.
//
// Backtracks the solver to the correct decision level and continues until no
// new conflicts or propagations are available in theory or in unit propagation
//

lbool LookaheadSMTSolver::laPropagateWrapper() {
    CRef cr;
    bool diff;
    do {
        diff = false;
        while ((cr = propagate()) != CRef_Undef) {
            if (decisionLevel() == 0)
                return l_False; // Unsat
            -- confl_quota;
#ifdef LADEBUG
            cerr << "; Got a conflict, quota now " << confl_quota.getQuota() << "\n";
#endif
            if (confl_quota <= 0)
                return l_Undef;

            vec<Lit> out_learnt;
            int out_btlevel;
            analyze(cr, out_learnt, out_btlevel);
#ifdef LADEBUG
            printf("Conflict: I would need to backtrack from %d to %d\n", decisionLevel(), out_btlevel);
#endif
            cancelUntil(out_btlevel);
            assert(value(out_learnt[0]) == l_Undef);
            if (out_learnt.size() == 1) {
                uncheckedEnqueue(out_learnt[0]);
            } else {
                CRef crd = ca.alloc(out_learnt, {true, computeGlue(out_learnt)});
                learnts.push(crd);
                attachClause(crd);
                uncheckedEnqueue(out_learnt[0], crd);
            }
            diff = true;
        }
        if (!diff) {
            TPropRes res = checkTheory(true);
            if (res == TPropRes::Unsat) {
#ifdef LADEBUG
                printf("Theory unsatisfiability\n");
#endif
                return l_False; // Unsat
            }
            else if (res == TPropRes::Propagate) {
#ifdef LADEBUG
                printf("Theory propagation / conflict\n");
#endif
                diff = true;
                -- confl_quota;
#ifdef LADEBUG
                cerr << "; Got a theory conflict, quota now " << confl_quota.getQuota() << "\n";
#endif
                if (confl_quota <= 0)
                    return l_Undef;
            }
        }
    } while (diff);

    return l_True;
}

/**
 * Set solver decision stack according to the path from the root to @param n.
 * As a side-effect the solver is either
 * (i)  set to the path, or
 * (ii) a node in the path is marked closed, meaning that there are no solutions in extensions of the path.
 * In case (i) @return pathbuild_success
 * In case (ii), either @return pathbuild_tlunsat or @return pathbuild_unsat
 *
 */
LookaheadSMTSolver::PathBuildResult LookaheadSMTSolver::setSolverToNode(LANode const & n) {
    cancelUntil(0);

    vec<Lit> path;
    LANode const * curr = &n;
    LANode const * parent = n.p;
    // Collect the truth assignment.
    while (parent != curr) {
        path.push(curr->l);
        curr = parent;
        parent = curr->p;
    }
#ifdef LADEBUG
    printf("Setting solver to the right dl %d\n", path.size());
#endif
    for (int i = path.size() - 1; i >= 0; i--) {
        newDecisionLevel();
        if (value(path[i]) == l_Undef) {
#ifdef LADEBUG
            printf("I will propagate %s%d\n", sign(path[i]) ? "-" : "", var(path[i]));
#endif
            int curr_dl = decisionLevel();
            uncheckedEnqueue(path[i]);
            lbool res = laPropagateWrapper();
            // Here it is possible that the solver is on level 0 and in an inconsistent state.  How can I check this?
            if (res == l_False) {
                return PathBuildResult::pathbuild_tlunsat; // Indicate unsatisfiability
            } else if (res == l_Undef) {
                cancelUntil(0);
                return PathBuildResult::pathbuild_restart; // Do a restart
            }
            if (curr_dl != decisionLevel()) {
                return PathBuildResult::pathbuild_unsat;
            }
        } else {
#ifdef LADEBUG
            printf("Would propagate %s%d but the literal is already assigned\n", sign(path[i]) ? "-" : "", var(path[i]));
#endif
            if (value(path[i]) == l_False) {
#ifdef LADEBUG
                printf("Unsatisfiable branch since I'd like to propagate %s%d but %s%d is assigned already\n", sign(path[i]) ? "-" : "", var(path[i]), sign(~path[i]) ? "-" : "", var(path[i]));
                printf("Marking the subtree false:\n");
                n->print();
#endif
                return PathBuildResult::pathbuild_unsat;
            } else {
                assert(value(path[i]) == l_True);
            }
        }
    }
    return PathBuildResult::pathbuild_success;
}

LookaheadSMTSolver::laresult LookaheadSMTSolver::expandTree(LANode & n, std::unique_ptr<LANode> c1, std::unique_ptr<LANode> c2)
{
    assert(c1);
    assert(c2);
    // Do the lookahead
    assert(decisionLevel() == n.d);
    auto [res, best] = lookaheadLoop();
    assert(decisionLevel() <= n.d);
    if (res != laresult::la_ok)
        return res;

    assert(best != lit_Undef);

    c1->p = &n;
    c1->d = n.d+1;
    c1->l = best;
    c2->p = &n;
    c2->d = n.d+1;
    c2->l = ~best;
    n.c1 = std::move(c1);
    n.c2 = std::move(c2);

    return laresult::la_ok;
}

LookaheadSMTSolver::LALoopRes LookaheadSMTSolver::solveLookahead() {
    struct PlainBuildConfig {
        bool stopCondition(LANode &, int) { return false; }
        LALoopRes exitState() const { return LALoopRes::unknown; }
    };
    return buildAndTraverse<LANode, PlainBuildConfig>(PlainBuildConfig()).first;
};

std::pair<LookaheadSMTSolver::laresult,Lit> LookaheadSMTSolver::lookaheadLoop() {
    ConflQuota prev = confl_quota;
    confl_quota = ConflQuota(); // Unlimited;
    if (laPropagateWrapper() == l_False) {
#ifdef LADEBUG
        printf("Already unsatisfiable at entering the lookahead loop\n");
#endif
        return {laresult::la_tl_unsat, lit_Undef};
    }
    confl_quota = prev;

    score->updateRound();
    int i = 0;
    int d = decisionLevel();

    int count = 0;
    bool respect_logic_partitioning_hints = config.respect_logic_partitioning_hints();
    int skipped_vars_due_to_logic = 0;

#ifdef LADEBUG
    printf("Starting lookahead loop with %d vars\n", nVars());
#endif
    for (Var v(idx % nVars()); !score->isAlreadyChecked(v); v = Var((idx + (++i)) % nVars()))
    {
        if (!decision[v]) {
            score->setChecked(v);
#ifdef LADEBUG
            cout << "Not a decision variable: " << v << "(" << theory_handler.getLogic.printTerm(theory_handler.varToTerm(v)) << ")\n";
#endif
            continue;
        }
        if (v == (idx * nVars()) && skipped_vars_due_to_logic > 0)
            respect_logic_partitioning_hints = false; // Allow branching on these since we looped back.
        if (respect_logic_partitioning_hints && !okToPartition(v)) {
            skipped_vars_due_to_logic ++;
            std::cout << "Skipping " << v << " since logic says it's not good\n";
            continue; // Skip the vars that the logic considers bad to split on
        }
#ifdef LADEBUG
        printf("Checking var %d\n", v);
#endif
        Lit best = score->getBest();
        if (value(v) != l_Undef || (best != lit_Undef && score->safeToSkip(v, best))) {
#ifdef LADEBUG
            printf("  Var is safe to skip due to %s\n",
                   value(v) != l_Undef ? "being assigned" : "having low upper bound");
#endif
            score->setChecked(v);
            // It is possible that all variables are assigned here.
            // In this case it seems that we have a satisfying assignment.
            // This is in fact a debug check
            if (static_cast<unsigned int>(trail.size()) == dec_vars) {
#ifdef LADEBUG
                printf("All vars set?\n");
#endif
                if (checkTheory(true) != TPropRes::Decide)
                    return {laresult::la_tl_unsat, lit_Undef}; // Problem is trivially unsat
                assert(checkTheory(true) == TPropRes::Decide);
#ifndef NDEBUG
                for (CRef cr : clauses) {
                    Clause& c = ca[cr];
                    unsigned k;
                    for (k = 0; k < c.size(); k++) {
                        if (value(c[k]) == l_True) {
                            break;
                        }
                    }
                    assert(k < c.size());
                }
#endif
                return {laresult::la_sat, lit_Undef}; // Stands for SAT
            }
            continue;
        }
        if (trail.size() == nVars() + skipped_vars_due_to_logic) {
            std::cout << "; " << skipped_vars_due_to_logic << " vars were skipped\n";
            respect_logic_partitioning_hints = false;
            continue;
        }
        count++;
        int p0 = 0, p1 = 0;
        for (int p : {0, 1}) { // for both polarities
            assert(decisionLevel() == d);
            double ss = score->getSolverScore(this);
            newDecisionLevel();
            Lit l = mkLit(v, p);
#ifdef LADEBUG
           printf("Checking lit %s%d\n", p == 0 ? "" : "-", v);
#endif
            uncheckedEnqueue(l);
            lbool res = laPropagateWrapper();
            if (res == l_False) {
                return {laresult::la_tl_unsat, lit_Undef};
            }
            else if (res == l_Undef) {
                cancelUntil(0);
                return {laresult::la_restart, lit_Undef};
            }
            // Else we go on
            if (decisionLevel() == d+1) {
#ifdef LADEBUG
                printf(" -> Successfully propagated %d lits\n", trail.size() - tmp_trail_sz);
#endif
                score->updateSolverScore(ss, this);
            } else if (decisionLevel() == d) {
#ifdef LADEBUG
                printf(" -> Propagation resulted in backtrack\n");
#endif
                score->updateRound();
                break;
            } else {
#ifdef LADEBUG
                printf(" -> Propagation resulted in backtrack: %d -> %d\n", d, decisionLevel());
#endif
                // Backtracking should happen.
                return {laresult::la_unsat, lit_Undef};
            }
            p == 0 ? p0 = ss : p1 = ss;
            // Update also the clause deletion heuristic?
            cancelUntil(decisionLevel() - 1);
        }
        if (value(v) == l_Undef)
        {
#ifdef LADEBUG
           printf("Updating var %d to (%d, %d)\n", v, p0, p1);
#endif
            score->setLAValue(v, p0, p1);
            score->updateLABest(v);
        }
    }
    Lit best = score->getBest();
    if (static_cast<unsigned int>(trail.size()) == dec_vars && best == lit_Undef) {
#ifdef LADEBUG
        printf("All variables are already set, so we have nothing to branch on and this is a SAT answer\n");
#endif
        return {laresult::la_sat, best};
    }
    assert(best != lit_Undef);
#ifdef LADEBUG
    printf("Lookahead phase over successfully\n");
    printf("Best I found propagates high %d and low %d\n",
           LAexacts[var(best)].getEx_h(),
           LAexacts[var(best)].getEx_l());
#endif
    idx = (idx + i) % nVars();
    if (!okToPartition(var(best))) { unadvised_splits++; }
    return {laresult::la_ok, best};
}
