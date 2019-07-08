//
//  wide
//
//  Created by @pd3v_
//

#include "expresstime.h"
//#include <functional>
//#include <stdlib.h>
#include <math.h>
//#include <iostream>
#include <unordered_map>

extern std::unordered_map<int,int> instsSteps;

int ExpressTime::whenMod(int count_turn) {
  step++;
  return step%count_turn;
}

int ExpressTime::revWhenMod(int count_turn) {
  return count_turn-whenMod(count_turn);
}

int ExpressTime::midWhenMod(int count_turn) {
  return whenMod(count_turn) <= count_turn/2 ? whenMod(count_turn-1)*2 : revWhenMod(count_turn)*2;
}

int ExpressTime::sine(float spread, float to) {
  return trunc(sinf(midWhenMod(spread)*PI/180)*spread);
}

int ExpressTime::saw(float spread, float to) {
  return trunc(whenMod(spread)*(to/spread));
}

int ExpressTime::saw(float spread, float from, float to) {
  float ascending = from <= to ? true : false;
  float value;
  
  if (ascending) {
    value = trunc(whenMod(spread)*((to-from)/spread));
  } else {
    value = trunc(revWhenMod(spread)*((from-to)/spread));
  }
  
  return (int) value;
}

int ExpressTime::tri(float spread, float from, float to) {
  float value = saw(spread, from, to*2);
  
  return (int) (trunc(to-from) >= value) ? value : saw(spread, to*2, from);
}

int ExpressTime::square(float spread, float from, float to) {
  float ascending = from <= to ? true : false;
  float value = saw(spread, from, to);
 
  if (ascending) {
    value = (trunc((to-from)/2) >= value) ? from : to;
  } else {
    std::swap(from, to);
    value = (trunc((to-from)/2) >= value) ? to : from;
  }
  
  return (int) value;
}

