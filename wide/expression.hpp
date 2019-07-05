//
//  wide
//
//  Created by @pd3v_
//

#include <math.h>
#include <vector>

using namespace std;

typedef vector<int> scale_t, chord_t;

float rnd(int max) {
  return (rand()%max)*1.;
}

float rnd(int min, int max) {
  return rand()%(max-min)+min;
}

float rnd(vector<int> range) {
  return range[rand()%range.size()];
}

float rnd(vector<int> range, int min, int max) {
  return range[rand()%(max-min)+min];
}

float range(float value, float min, float max, float toMin, float toMax) {
  return fabsf((value-min)/(max-min)*(toMax-toMin)+toMin);
}

// ====================================
// Scales

scale_t Chromatic  {0,1,2,3,4,5,6,7,8,9,10,11};
scale_t Maj        {0,2,4,5,7,9,11};
scale_t Min        {0,2,3,5,7,9,11};

//Chords

chord_t cMaj  {0,4,7};
chord_t cMin  {0,3,7};


