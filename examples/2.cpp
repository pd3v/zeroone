// Play random notes from chromatic scale; default scale for every instrument

i(1).play(n(
  {rnd(12)},
  1,
  {16},
  4
))

// Random chords on varying repeating rhythm

scale CMajor {0,2,4,5,7,9,11}

i(1).play(n(
  ({rnd(5),rnd(5,8),rnd(8,12)}), // chromatic scale
  1,
  ({4,6,32,3}),
  4
))

i(2).play(n(
  {rnd(CMajor)},
  1,
  ({4,6,32,3}),
  5
))