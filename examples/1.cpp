
// Play C4 note in instrument 0, amplitude 1, 1/4 duration notes, octave 4.

i(0).play(
  n(
    ({0}),1,4,4
  ))

// Play C Major 7 chord in instrument 0, amplitude 1, 1/16 duration notes, octave 5.

i(0).play(
  n(
    ({0,2,4,6}),1,16,5
  ))

// Or

chordType Maj7 {0,2,4,6};

i(0).play(
  n(
    (Maj7),1,4,5
  ))
