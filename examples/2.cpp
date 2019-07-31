// Play random notes from C Major scale; default scale for every instrument

i(0).play(
  n(
    ({rnd(7)}),1,16,4
  ))

i(0).scale() // (std::vector<int>) { 0, 2, 4, 5, 7, 9, 11 }

// Set scale to Minor or Chromatic

i(0).scale(Minor)
i(0).scale(Chromatic)

// Random through scale varying number of notes

i(0).play(
  n(
    ({rnd<int>(i(0).scale().size())}),1,32,5
  ))