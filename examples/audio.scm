(def synth
  (def source (audio "source.flac" 90.0))
  (def lfo (+ 6.0 (* 4.0 (sine 0.04))))
  (def carrier (sine (midi->freq (+ 60.0 (* 24.0 (sine 0.23)))) (* (ref lfo) (ref source))))
  (def output (+
    (highpass (ref carrier) 650.0 4.0)
    (lowpass (ref source) 650.0 4.0))))
