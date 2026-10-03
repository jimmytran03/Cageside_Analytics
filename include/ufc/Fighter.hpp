#pragma once
#include <string>

namespace ufc {

struct Fighter {
    std::string name;
    double rating = 1500.0;
    int wins = 0;
    int losses = 0;
};

} 
