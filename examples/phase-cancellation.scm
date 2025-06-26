(synth
  (def freq 55.0)
  (def b-freq (+ (ref freq) (* 0.5 (sine :freq 0.06 :phase 0.0))))
  (def a (sine :freq (ref freq) :phase 0.0))
  (def b (sine :freq (ref b-freq) :phase 3.14))
  (def output (+ (* 0.5 (ref a)) (* 0.5 (ref b)))))
