(synth
  (def threshold-freq (sine :freq 0.06 :phase 0.0))
  (def threshold (+ 0.5 (* 0.5 (sine :freq (* 8.0 (ref threshold-freq)) :phase 0.0))))
  (def freq (+ 85.0 (* 8 (sine :freq 0.26 :phase 0.0))))
  (def output (bitcrush (hardclip (ref threshold) (* 1.0 (sine :freq (ref freq) :phase 0.0))) :target_freq 44100 :bits 7)))
