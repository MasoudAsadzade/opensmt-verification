(set-logic QF_UF)
(set-info :status sat)
(declare-sort U 0)
(declare-const a U)
(declare-fun f (U) U)
(declare-fun x () U)
(assert (= x (f a)))
(check-sat)
(exit)