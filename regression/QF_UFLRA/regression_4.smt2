(set-logic QF_UFLRA)
(set-info :status unsat)
(declare-fun f1 (Real) Real)
(declare-fun x () Real)
(assert (and (= 0.0 x) (> (f1 x) 1) (= 0.0 (f1 0.0))))
(check-sat)