. wide operator nonsynth-0.4.pd pdlc5.cpp

cling -std=c++14

.L wideAPI
#include "../src/wide.h"

wide()
on()
off()
.q
clear
ah
instson()
isinst(1)
TaskPool<SJob>::isRunning
TaskPool<CCJob>::isRunning
TaskPool<CCJob>::jobs.size()
isync(1)
i1 
insts[0]

isinstaddress(1).outDur(0)
insts[0].play(n({56},1,{1}))