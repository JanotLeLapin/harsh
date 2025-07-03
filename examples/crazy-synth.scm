(synth
  (def mod (sine (* 10.0 (sine 0.01))))
  (def noise (noise 777.0))
  (def signal (* (* 0.5 (ref mod)) (square 55.0 (* (ref noise) (* 0.2 (sine 0.1))))))
  (def bitdepth (* 10.0 (sine 0.1 1.5)))
  (def output (bitcrush (ref signal) :target_freq 16000.0 :bits (ref bitdepth))))
