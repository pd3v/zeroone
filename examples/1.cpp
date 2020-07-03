
// Play C4 note in instrument 1, amplitude 1, 1/4 duration notes, octave 4.

i(1).play(n(
  {0},
  1,
  {4},
  4
))

// Play C Major 7 chord in instrument 1, amplitude 1, 1/16 duration notes, octave 5.

i(1).play(n(
  ({0,4,7,11}),
  1,
  {16},
  5
))

// Or

chord CMaj7 {0,4,7,11}

i(1).play(n(
  (CMaj7),
  1,
  {4},
  5
))