(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (and (<= y (* 2 x)) (>= y (* 2 x))))
(assert (and (<= y (+ 1 (* 2 z))) (>= y (+ 1 (* 2 z)))))
(check-sat)