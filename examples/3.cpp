// randomize anything and everything

i(0).play(
  n(
    ({rnd<int>(i(0).scale().size())}),
    rnd<float>(0,1),
    rndBunch<unsigned long>({4,3,8,16}),
    rndBunch<int>({2,5,6}))
)

// same as

i(0).play(
  n(
    ({rndBunchNotes({0,2,4,5,7,9,11})}),
    rnd<float>(0,1),
    rndBunchDur({4,3,8,16}),
    rndBunchOct({2,5,6}))
)