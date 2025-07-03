(synth
  (def sine-freq (* (+ 1600.0 (* 700.0 (sine 0.1))) (+ 0.5 (* 0.5 (noise 5)))))
  (def sine-phase (* 64.0 (sine 0.1)))
  (def output (* 0.06 (sine (ref sine-freq) (ref sine-phase)))))
