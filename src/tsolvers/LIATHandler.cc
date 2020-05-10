#include "LIATHandler.h"
#include "TreeOps.h"
#include <liasolver/LIASolver.h>

LIATHandler::LIATHandler(SMTConfig& c, LIALogic& l, vec<DedElem>& d, TermMapper& tmap)
        : TSolverHandler(c, d, tmap)
        , logic(l)
{
    liasolver = new LIASolver(config, logic, deductions);
    SolverId my_id = liasolver->getId();
    tsolvers[my_id.id] = liasolver;
    solverSchedule.push(my_id.id);
}

LIATHandler::~LIATHandler() { }

Logic &LIATHandler::getLogic()
{
    return logic;
}

const Logic &LIATHandler::getLogic() const
{
    return logic;
}


void LIATHandler::fillTmpDeds(PTRef root, Map<PTRef,int,PTRefHash> &refs)
{
    // XXX Reorganize so that the storing of the previous variable would
    // not be so awkward?
    vec<PtChild> terms;
    getTermList(root, terms, getLogic());

    for (int i = 0; i < terms.size(); i++)
    {
        PTRef tr = terms[i].tr;
        if (logic.isNumLeq(tr)) {
            if (!refs.has(tr)) {
                declareAtom(tr);
                Var v = tmap.addBinding(tr);
                while (deductions.size() <= v)
                    deductions.push({liasolver->getId(), l_Undef});
                refs.insert(tr, v);
            }
        }
        else if (logic.isNumEq(tr)) {
            vec<PTRef> args;
            Pterm& p = logic.getPterm(tr);
            args.push(p[0]);
            args.push(p[1]);
            PTRef i1 = logic.mkNumLeq(args);
            PTRef i2 = logic.mkNumGeq(args);
            // These can simplify to true and false, and we don't
            // want them to LRA solver
            if (!refs.has(i1) && logic.isNumLeq(i1)) {
                declareAtom(i1);
                Var v = tmap.addBinding(i1);
                while (deductions.size() <= v)
                    deductions.push(DedElem(liasolver->getId(), l_Undef));
                refs.insert(i1, v);
            }
            if (!refs.has(i2) && logic.isNumLeq(i2)) {
                declareAtom(i2);
                Var v = tmap.addBinding(i2);
                while (deductions.size() <= v)
                    deductions.push(DedElem(liasolver->getId(), l_Undef));
                refs.insert(i2, v);
            }
        } else {
            // A variable.  Will be handled as a part of an equality or
            // inequality.
            // assert(false); // Not an equality or inequality
        }
    }
}

bool LIATHandler::assertLit_special(PtAsgn a)
{
//    assert(logic.isRealEq(a.tr) || logic.isRealLeq(a.tr));
    assert(a.sgn == l_True);
    if (logic.isNumEq(a.tr)) {
        Pterm& p = logic.getPterm(a.tr);
        vec<PTRef> args;
        args.push(p[0]);
        args.push(p[1]);
        PTRef i1 = logic.mkNumLeq(args);
        PTRef i2 = logic.mkNumGeq(args);
        bool res = assertLit(PtAsgn(i1, l_True));
        return res && assertLit(PtAsgn(i2, l_True));
    }
    else {
        return assertLit(a);
    }
}

#ifdef PRODUCE_PROOF

PTRef LIATHandler::getInterpolant(const ipartitions_t& mask, map<PTRef, icolor_t> *labels)
{
    throw std::logic_error{"Interpolation currently not supported in LIA"};
}
#endif // PRODUCE_PROOF
