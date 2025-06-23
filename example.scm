(synth
  (def carrier (sine :freq 440.0 :phase 0.0))
  (* (ref carrier) 0.5))
