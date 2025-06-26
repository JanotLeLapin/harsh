(synth
  (def carrier-freq (+ 220.0 (* 113.5 (sine :freq 0.04 :phase 0.0))))
  (def target-freq (+ 1200 (* 600 (sine :freq 0.1 :phase 0.0))))
  (def output (< 0 (bitcrush (square :freq (ref carrier-freq) :phase 0.0) :target_freq (ref target-freq) :bits 32))))
