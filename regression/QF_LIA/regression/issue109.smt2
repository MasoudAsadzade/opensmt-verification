(set-logic QF_LIA)
(declare-fun v5 () Bool)
(declare-fun i3 () Int)
(declare-fun i4 () Int)
(assert (distinct (- (/ 55 93) i3) 96))
(assert (or (distinct (- (/ 55 93) i3) i4) v5))
(check-sat)
