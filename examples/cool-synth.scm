(synth
  (def freq 110.0)
  (def mod (sine (* (sine 0.01) (ref freq))))
  (def carrier (sine (ref freq) (ref mod)))
  (def output (* (ref carrier) 0.2)))
