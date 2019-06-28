//
//  wide
//
//  Created by @pd3v_
//

#pragma once

#define PI 3.1415926535

#include <vector>

class Expression {
public:
  static int rnd(int max);
  static int rnd(int min, int max);
  static int rnd(std::vector<int> range);
  static int rnd(std::vector<int> range, int min, int max);
  static int whenMod(int count_turn);
  static int revWhenMod(int count_turn);
  static int midWhenMod(int count_turn);
  static int range(float value, float min, float max, float toMin, float toMax);
  static int sine(float spread, float to);
  static int saw(float spread, float to);
  static int saw(float spread, float from, float to);
  static int sawi(int to);
  static int square(float spread, float from, float to);
  static int tri(float spread, float from, float to);
  
  static unsigned long int step;
};