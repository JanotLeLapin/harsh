(synth
  (def pitch-jump (* -2.0 (square :freq 0.6)))
  (def pitch (midi->freq (+ 72.0 (ref pitch-jump) (* 8.0 (sine :freq 0.08 :phase 5.15)))))
  (def output (hardclip (+ 0.9 (* 0.05 (sine :freq 1.0))) (diode (sine :freq (ref pitch))))))
