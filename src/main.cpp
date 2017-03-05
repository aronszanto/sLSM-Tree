//
//  main.cpp
//  lsm-tree
//
//  Created by Aron Szanto on 3/2/17.
//  Copyright © 2017 Aron Szanto. All rights reserved.
//

#include <stdio.h>
#include <cstdio>
#include "skipList.hpp"
#include "lsm.hpp"


int main(){
    auto sl = lsm::SkipList<int32_t, int32_t, 16>(INT32_MIN,INT32_MAX);
    sl.insert_key(5, 10);
    std::printf("%i\n", sl.lookup(5));
    auto lsm = lsm::LSM<int32_t, int32_t>();
    return 0;
    
}
