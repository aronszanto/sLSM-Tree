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
#include <math.h>
#include <random>

#include "skipList.hpp"
#include "bloom.hpp"
#include "lsm.hpp"


using namespace std;


void bloomFilterTest(){
    std::random_device                  rand_dev;
    std::mt19937                        generator(rand_dev());
    std::uniform_int_distribution<int>  distribution(INT32_MIN, INT32_MAX);
  
    const int num_inserts = 100000;
    double fprate = .04;
    double denom = 0.480453013918201; // (ln(2))^2
    uint64_t size = -1 * num_inserts * (log(fprate) / denom);
    
    double ln2 = 0.693147180559945;
    
    uint8_t numHashes = (int) ceil( (size / num_inserts) * ln2);  // ln(2)
    
    printf("%llu,%i\n", size, numHashes);
    BloomFilter<int32_t> bf = BloomFilter<int32_t>(size, numHashes);
    
    std::vector<int> to_insert;
    for (int i = 0; i < num_inserts; i++) {
        int insert = distribution(generator);
        to_insert.push_back(insert);
    }
    std::clock_t    start_insert;
    std::cout << "Starting inserts" << std::endl;
    start_insert = std::clock();
    for (int i = 0; i < num_inserts; i++) {
        bf.add(&i, sizeof(i));
    }
    
    double total_insert = (std::clock() - start_insert) / (double)(CLOCKS_PER_SEC);
    
    std::cout << "Time: " << total_insert << " s" << std::endl;
    std::cout << "Inserts per second: " << (int) num_inserts / total_insert << " s" << std::endl;
    int fp = 0;
    for (int i = num_inserts; i < 2 * num_inserts; i++) {
        bool lookup = bf.mayContain(&i, sizeof(i));
        if (lookup){
            cout << i << " found but didn't exist" << endl;
            fp++;
        }
    }
    cout << fp << endl;
    cout << "FP rate: " << ((double) fp / double(num_inserts)) << endl;

    
    
    
    
    
}
void insertLookupTest(){
    std::random_device                  rand_dev;
    std::mt19937                        generator(rand_dev());
    std::uniform_int_distribution<int>  distribution(INT32_MIN, INT32_MAX);
    
    
    const int num_inserts = 900000;
    const int max_levels = 16;
    const int num_runs = 9;
    const int total_size = num_inserts * sizeof(int);
    const int run_size = total_size / num_runs;
    SkipList<int32_t, int32_t, max_levels>(INT32_MIN,INT32_MAX);
    LSM<int32_t, int32_t> lsmTree = LSM<int32_t, int32_t>(total_size, run_size, 2);
    
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

}

int main(){

    
    bloomFilterTest();
    return 0;
    
}
