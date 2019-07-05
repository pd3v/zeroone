//
//  wide
//
//  Created by @pd3v_
//

#pragma once

#define PI 3.1415926535

#include <vector>
#include <functional>

class ExpressTime {
public:
  int whenMod(int count_turn);
  int revWhenMod(int count_turn);
  int midWhenMod(int count_turn);
  int range(float value, float min, float max, float toMin, float toMax);
  int sine(float spread, float to);
  int saw(float spread, float to);
  int saw(float spread, float from, float to);
  int sawi(int to);
  int square(float spread, float from, float to);
  int tri(float spread, float from, float to);
  
  unsigned long int step = 0;
};