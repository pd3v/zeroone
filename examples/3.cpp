
bpm(140)

scale CMinor{0,2,3,5,7,8,10}

// randomize anything and everything

i(1).play(n(
  ({rnd(CMinor)}),
  rnd(0.0,1.0),
  {rnd({4,3,8,16})},
  rnd({2,5,6}))
)