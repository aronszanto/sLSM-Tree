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
    
    const int num_inserts = 10000;
    const int max_levels = 16;
    lsm::SkipList<int32_t, int32_t, max_levels> sl = lsm::SkipList<int32_t, int32_t, max_levels>(INT32_MIN,INT32_MAX);
    
    std::vector<int> to_insert;
    for (int i = 0; i < num_inserts; i++) {
        to_insert.push_back(distribution(generator));
    }
    std::clock_t    start_insert;
    std::cout << "Starting inserts" << std::endl;
    start_insert = std::clock();
    for (int i = 0; i < num_inserts; i++) {
        sl.insert_key(to_insert[i], i);
    }
    double total_insert = (std::clock() - start_insert) / (double)(CLOCKS_PER_SEC);
    
    std::cout << "Time: " << total_insert << " s" << std::endl;
    std::cout << "Inserts per second: " << num_inserts / total_insert << " s" << std::endl;
    
    
    
//    auto lsm = lsm::LSM<int32_t, int32_t>();
    return 0;
    
}
