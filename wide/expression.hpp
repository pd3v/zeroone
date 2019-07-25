//
//  wide
//
//  Created by @pd3v_
//

#include <vector>
#include <functional>
#include <algorithm>

using namespace std;

typedef vector<int> scaleType, chordType;

template <typename T>
const T rnd(const T& max) {
  return rand()%static_cast<int>(max);
};

template <typename T>
const T range(const T& value,const T& max,const T& toMin,const T& toMax) {
  return value/max*(toMax-toMin)+toMin;
}

template <typename T>
const T rnd(const T& min,const T& max) {
  if (min >= 0 && max <= 1)
    return range(static_cast<float>(rand()%10), 10.f, min, max);
  else
    return rand()%static_cast<int>(max-min)+min;
};

const int rndBunch(const vector<int> bunch) {
  return static_cast<int>(bunch[rand()%bunch.size()]);
}

const unsigned long rndBunchDur(const vector<unsigned long> bunch) {
  return static_cast<unsigned long>(bunch[rand()%bunch.size()]);
}

const float rndBunchAmp(const vector<float> bunch) {
  return static_cast<float>(bunch[rand()%bunch.size()]);
}

auto rndBunchNotes = rndBunch;
auto rndBunchOct = rndBunch;

template <typename T>
const vector<T> scramble(vector<T> n) {
  random_shuffle(n.begin(),n.end());
  
  return n;
}

bool whenMod(int countTurn, unsigned long step) {
  return step%countTurn == 0;
}

vector<int> thisthat(unsigned long step, function<vector<int>()> thisFunc, function<vector<int>()> thatFunc, function<bool()> pred) {
  return pred() ? thisFunc() : thatFunc();
}

int thisthat(unsigned long step, function<int()> thisFunc, function<int()> thatFunc, function<bool()> pred) {
  return pred() ? thisFunc() : thatFunc();
}

// Not viable
/*vector<int> shuffleoct(vector<int> vect,const vector<int> octaves) {
  transform(vect.begin(),vect.end(),vect.begin(),[&octaves](int n){
    n = octaves[rand()%octaves.size()]*12+n;
    return n;
  });
  return vect;
}*/

template <typename T>
T cycle(vector<T> v, unsigned long step) {
  return v.at(step%v.size());
}

vector<int> rotR(vector<int>& notes, vector<int> scale) {
  transform(notes.begin(),notes.end(),notes.begin(),[&scale](int note){
    note += 1;
    note %= scale.size();
    
    return note;
  });
  
  return notes;
}

vector<int> rotL(vector<int>& notes, vector<int> scale) {
  transform(notes.begin(),notes.end(),notes.begin(),[&scale](int note){
    note -= 1;
    note %= static_cast<int>(scale.size());
    
    if (note < 0)
      note = static_cast<int>(scale.size()-1);
  
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
