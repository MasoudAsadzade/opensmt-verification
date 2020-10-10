#ifndef OPENSMT_RDLTHANDLER_H
#define OPENSMT_RDLTHANDLER_H

#include "TSolverHandler.h"

class LRALogic;
class RDLSolver;

class RDLTHandler : public TSolverHandler
{
private:
    LRALogic& logic;
    RDLSolver *rdlsolver;
public:
    RDLTHandler(SMTConfig &c, LRALogic &l, TermMapper &tmap);
    virtual ~RDLTHandler() = default;
    Logic &getLogic() override;
    const Logic &getLogic() const override;
    PTRef getInterpolant(const ipartitions_t &mask, map<PTRef, icolor_t>*) override {
        throw std::logic_error("Not implemented yet");
    }

};


#endif //OPENSMT_RDLTHANDLER_H