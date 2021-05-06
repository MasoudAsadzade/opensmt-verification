(set-logic QF_UF)
(set-info :status unsat)
(declare-fun uf6 (Bool) Bool)
(declare-fun x () Bool)
(declare-fun y () Bool)
(declare-fun z () Bool)
(assert (uf6 x))
(assert (not (uf6 (or y z))))
(assert (not (uf6 (or y (not z)))))
(assert (not (uf6 (or (not y) z))))
(assert (not (uf6 (or (not y) (not z)))))
(check-sat)