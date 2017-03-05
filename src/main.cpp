//
//  main.cpp
//  lsm-tree
//
//  Created by Aron Szanto on 3/2/17.
//  Copyright © 2017 Aron Szanto. All rights reserved.
//

#include <stdio.h>
#include <cstdio>
#include <ctime>
#include <iostream>
#include "skipList.hpp"
#include "lsm.hpp"


int main(){
    
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(INT32_MIN,INT32_MAX);
    
    auto sl = lsm::SkipList<int32_t, int32_t, 16>(INT32_MIN,INT32_MAX);
    int num_inserts = 30000;
    std::vector<int> to_insert;
    for (int i = 0; i < num_inserts; i++) {
        to_insert.push_back(distribution(generator));
    }
    std::clock_t    start;
    std::cout << "Starting inserts" << std::endl;
    start = std::clock();
    for (int i = 0; i < num_inserts; i++) {
        sl.insert_key(to_insert[i], i);
    }

    std::cout << "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    
    
    
//    auto lsm = lsm::LSM<int32_t, int32_t>();
    return 0;
    
}
