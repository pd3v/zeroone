//
//  wide - Live coding DSLish API + MIDI sequencer
//
//  Created by @pd3v_
//

#pragma once

#include <iostream>
#include <vector>

struct Notes {
  std::vector<int> notes;
  double amp;
  std::vector<int> dur;
  int oct;
  std::vector<int> barDur; // holds original (bar units) dur
  
  void print() {
    std::cout << "{ ";
    for_each(notes.begin(),notes.end(),[](int n){std::cout << n << " ";});
    std::cout << "}";
    std::cout << amp << " { ";
    for_each(dur.begin(),dur.end(),[](int d){
      std::cout << d << " ";
    });
    std::cout << "}" << oct << " " << std::endl;
  }
};

struct CC {
  int ch;
  int value;
  void print() {
    std::cout << "cc { ";
    std::cout << ch << "," << value;
    std::cout << "}" << std::endl;
  }
};

