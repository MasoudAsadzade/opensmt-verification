(set-logic QF_AX)
(set-info :status sat)
(declare-sort I 0)
(declare-sort E 0)
(declare-fun a () (Array I E))
(declare-fun e () E)
(declare-fun i () I)
(assert (not (= a (store a i e))))
(check-sat)