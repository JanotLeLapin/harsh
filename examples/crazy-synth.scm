(synth
  (def mod (sine :freq (* 10.0 (sine :freq 0.01 :phase 0.0)) :phase 0.0))
  (def noise (noise :seed 777.0))
  (def signal (* (* 0.5 (ref mod)) (square :freq 55.0 :phase (* (ref noise) (* 0.2 (sine :freq 0.1 :phase 0.0))))))
  (def bitdepth (* 10.0 (sine :freq 0.1 :phase 1.5)))
  (def output (bitcrush (ref signal) :target_freq 16000.0 :bits (ref bitdepth))))
