(synth
  (def freq 110.0)
  (def mod (sine :freq (* (sine :freq 0.01 :phase 0.0) (ref freq)) :phase 0.0))
  (def carrier (sine :freq (ref freq) :phase (ref mod)))
  (def output (* (ref carrier) 0.2)))
