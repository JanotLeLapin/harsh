(synth
  (def freq 55.0)
  (def b-freq (+ (ref freq) (* 0.5 (sine 0.06))))
  (def a (sine (ref freq) 0.0))
  (def b (sine (ref b-freq) 3.14))
  (def output (+ (* 0.5 (ref a)) (* 0.5 (ref b)))))
