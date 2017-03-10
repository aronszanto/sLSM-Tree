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
//    weird level gen stuff...
    std::printf("%x\n",random());
    std::printf("%x\n",(1 << 16) - 1);

    
    //return 0;
    auto s =lsm::SkipList<int32_t, int32_t, 16>(INT32_MIN,INT32_MAX);
    int x[16];
    for (int i = 0; i < 1000000; i++){
        x[s.generateNodeLevel()] += 1;
    }
    
    for (int i = 0; i < 16; i++){
        std::cout << i << " " << x[i] << "\n";
    }
    return 0;
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(INT32_MIN,INT32_MAX);
    
    const int num_inserts = 20000;
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
    std::cout << sl.num_elements() << std::endl;
    
    
    
//    auto lsm = lsm::LSM<int32_t, int32_t>();
    return 0;
    
}
