(synth
  (def carrier-freq (+ 220.0 (* 113.5 (sine 0.04))))
  (def target-freq (+ 1200 (* 600 (sine 0.1))))
  (def output (< 0 (bitcrush (square (ref carrier-freq)) :target_freq (ref target-freq)))))
