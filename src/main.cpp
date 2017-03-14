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

using namespace std;
int main(){

    
    std::random_device                  rand_dev;
    std::mt19937                        generator(rand_dev());
    std::uniform_int_distribution<int>  distribution(INT32_MIN, INT32_MAX);

    
    const int num_inserts = 900000;
    const int max_levels = 16;
    const int num_runs = 9;
    const int total_size = num_inserts * sizeof(int);
    const int run_size = total_size / num_runs;
    lsm::SkipList<int32_t, int32_t, max_levels>(INT32_MIN,INT32_MAX);
    lsm::LSM<int32_t, int32_t> lsmTree = lsm::LSM<int32_t, int32_t>(total_size, run_size, 2);
    
    std::vector<int> to_insert;
    for (int i = 0; i < num_inserts; i++) {
        int insert = distribution(generator);
        to_insert.push_back(i);
    }
    std::clock_t    start_insert;
    std::cout << "Starting inserts" << std::endl;
    start_insert = std::clock();
    for (int i = 0; i < num_inserts; i++) {
        lsmTree.insert_key(to_insert[i], i);
    }
    
    double total_insert = (std::clock() - start_insert) / (double)(CLOCKS_PER_SEC);
    
    std::cout << "Time: " << total_insert << " s" << std::endl;
    std::cout << "Inserts per second: " << (int) num_inserts / total_insert << " s" << std::endl;
    std::cout << lsmTree.num_elements() << std::endl;
    
    for (int i = 0; i < num_inserts; i++) {
        int lookup = lsmTree.lookup(to_insert[i]);
        if (lookup != i)
            cout << "LOOKUP TEST FAILED ON ITERATION " << i << ". Got " << i << " but was expecting " << lookup << ".\n";
    }
    
    return 0;
    
}
