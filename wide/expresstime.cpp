//
//  wide
//
//  Created by @pd3v_
//

#include "expression.h"
#include <functional>
#include <stdlib.h>
#include <math.h>
#include <iostream>

typedef std::vector<int> scale_t, chord_t;

//unsigned long int Expression::step = 0;

int Expression::rnd(int max) {
  return (int) rand()%max;
}

int Expression::rnd(int min, int max) {
  return (int) rand()%(max-min)+min;
}

int Expression::rnd(std::vector<int> range) {
  return range[rand()%range.size()];
}

int Expression::rnd(std::vector<int> range, int min, int max) {
  return range[rand()%(max-min)+min];
}

//int Expression::whenMod(int count_turn) {
//  //return step%count_turn;
//}

int Expression::whenMod(int count_turn) {
  //static unsigned long int _step = 0;
  step++;
  return step%count_turn;
}

/*std::function<int(int)> Expression::whenMod(int count_turn) {
  auto fn = std::bind([&count_turn] (unsigned long int step) -> int {
    return step%count_turn;
  }, 0);

  return fn();
}*/

//int Expression::whenMod2(int count_turn, int step) {
//  auto newWhenMod = std::bind(whenMod,_1,step);
//  return step%count_turn;
//}

/*int Expression::revWhenMod(int count_turn) {
  return count_turn-whenMod(count_turn);
}*/

/*int Expression::midWhenMod(int count_turn) {
  return whenMod(count_turn) <= count_turn/2 ? whenMod(count_turn-1)*2 : revWhenMod(count_turn)*2;
}*/

int Expression::range(float value, float min, float max, float toMin, float toMax) {
  return (int) fabsf((value-min)/(max-min)*(toMax-toMin)+toMin);
}

/*int Expression::sine(float spread, float to) {
  return trunc(sinf(midWhenMod(spread)*PI/180)*spread);
}*/

/*int Expression::saw(float spread, float to) {
  return trunc(whenMod(spread)*(to/spread));
}*/

/*int Expression::saw(float spread, float from, float to) {
  float ascending = from <= to ? true : false;
  float value;
  
  if (ascending) {
    value = trunc(whenMod(spread)*((to-from)/spread));
  } else {
    value = trunc(revWhenMod(spread)*((from-to)/spread));
  }
  
  return (int) value;
}*/

/*int Expression::tri(float spread, float from, float to) {
  float value = saw(spread, from, to*2);
  
  return (int) (trunc(to-from) >= value) ? value : saw(spread, to*2, from);
}*/

/*int Expression::square(float spread, float from, float to) {
  float ascending = from <= to ? true : false;
  float value = saw(spread, from, to);
 
  if (ascending) {
    value = (trunc((to-from)/2) >= value) ? from : to;
  } else {
    std::swap(from, to);
    value = (trunc((to-from)/2) >= value) ? to : from;
  }
  
  return (int) value;
}*/

// ====================================
// Scales

scale_t chromatic  {0,1,2,3,4,5,6,7,8,9,10,11};
scale_t maj        {0,2,4,5,7,9,11};
scale_t min        {0,2,3,5,7,9,11};

//Chords

chord_t cMaj  {0,4,7};