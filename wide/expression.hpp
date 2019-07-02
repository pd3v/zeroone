//
//  wide
//
//  Created by @pd3v_
//

using namespace std;

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

