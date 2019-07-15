//
//  wide
//
//  Created by @pd3v_
//

#include <math.h>
#include <vector>
#include <chrono>
#include <functional>
#include <algorithm>

using namespace std;

typedef vector<int> scaleType, chordType;

template <typename T>
const T rnd(const T& max) {
  return rand()%static_cast<int>(max);
};

template <typename T>
const T rnd(const T& min,const T& max) {
  if (min >= 0 && max <= 1)
    return 1.f/(rand()%10);
  else
    return rand()%static_cast<int>(max-min)+min;
};

template <typename T>
const T rndBunch(const vector<T> bunch) {
  return bunch[rand()%bunch.size()];
}

template <typename T>
const T range(const T& value,const T& max,const T& toMin,const T& toMax) {
  return value/max*(toMax-toMin)+toMin;
}

std::vector<int> shuffleOct(vector<int> vect,const vector<int> octaves) {
  transform(vect.begin(),vect.end(),vect.begin(),[&](int n){
    return octaves[rand()%octaves.size()]*12+n;
  });

  return vect;
}

std::vector<int> rotR(vector<int>& notes) {
  transform(notes.begin(),notes.end(),notes.begin(),[](int note){
    note += 1;
    note %= 12;
    
    return note;
  });
  
  return notes;
}

std::vector<int> rotL(vector<int>& notes) {
  transform(notes.begin(),notes.end(),notes.begin(),[](int note){
    note -= 1;
    
    if (note > 12)
      note = 0;
    else if (note < 0)
      note = 11;
    
    return note;
  });
  
  return notes;
}

// ====================================

// Scales
scaleType Chromatic  {0,1,2,3,4,5,6,7,8,9,10,11};
scaleType Major      {0,2,4,5,7,9,11};
scaleType Minor      {0,2,3,5,7,9,11};

//Chords
chordType Maj  {0,2,4};
chordType Min  {0,1,4};
