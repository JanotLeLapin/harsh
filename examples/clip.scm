(synth
  (def threshold-freq (sine 0.06))
  (def threshold (+ 0.5 (* 0.5 (sine :freq (* 8.0 (ref threshold-freq))))))
  (def freq (+ 85.0 (* 8 (sine 0.26))))
  (def output (bitcrush (hardclip (* 1.0 (sine :freq (ref freq) :phase 0.0)) (ref threshold)) :bits 7)))
