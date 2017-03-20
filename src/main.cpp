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
  
    const int num_inserts = 10;
    double fprate = .1;
    BloomFilter<int32_t> bf = BloomFilter<int32_t>(num_inserts, fprate);
    
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
            // cout << i << " found but didn't exist" << endl;
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
    
    
    const int num_inserts = 500000;
    const int max_levels = 16;
    const int num_runs = 100;
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
    
    std::clock_t    start_lookup;
    std::cout << "Starting lookups" << std::endl;
    start_lookup = std::clock();

    for (int i = 0 ; i < num_inserts; i++) {
        int lookup = lsmTree.lookup(to_insert[i]);
        if (lookup != i)
            cout << "LOOKUP TEST FAILED ON ITERATION " << i << ". Got " << lookup << " but was expecting " << to_insert[i] << ".\n";
    }
    double total_lookup = (std::clock() - start_lookup) / (double)(CLOCKS_PER_SEC);

    std::cout << "Time: " << total_lookup << " s" << std::endl;
    std::cout << "Lookups per second: " << (int) num_inserts / total_lookup << " s" << std::endl;
}
void runInOrderTest() {
    const int num_inserts = 100;
    const int max_levels = 16;
    const int num_runs = 10;
    const int total_size = num_inserts * (2 * sizeof(int));
    const int run_size = total_size / num_runs;
    SkipList<int32_t, int32_t, max_levels>(INT32_MIN,INT32_MAX);
    LSM<int32_t, int32_t> lsmTree = LSM<int32_t, int32_t>(total_size, run_size, 2);
    
    std::vector<int> to_insert;
    for (int i = 0; i < num_inserts; i++) {
        to_insert.push_back(i);
    }
    std::clock_t    start_insert;
    std::cout << "Starting inserts" << std::endl;
    start_insert = std::clock();
    for (int i = 0; i < num_inserts; i++) {
        lsmTree.insert_key(to_insert[i], i);
    }
    
    for (int i = 0; i < num_runs; i++){
        cout << "on run " << i << endl;
        auto all = lsmTree.C_0[i]->get_all();
        
        for (int j = 0; j < lsmTree._eltsPerRun; j++){
            
            auto kv = all[j];
            cout << "K: " << kv.key << ", " << "V: " << kv.value << endl;
        }
    }
}

void diskLevelTest(){
    const int num_inserts = 10000;
    const int max_levels = 16;
    const int num_runs = 1;
    const int total_size = num_inserts * (2 * sizeof(int));
    const int run_size = total_size / num_runs;
    SkipList<int32_t, int32_t, max_levels>(INT32_MIN,INT32_MAX);
    LSM<int32_t, int32_t> lsmTree = LSM<int32_t, int32_t>(total_size, run_size, 2);
    
    std::vector<int> to_insert;
    for (int i = 0; i < num_inserts; i++) {
        to_insert.push_back(i);
    }
    std::clock_t    start_insert;
    std::cout << "Starting inserts" << std::endl;
    start_insert = std::clock();
    for (int i = 0; i < num_inserts; i++) {
        lsmTree.insert_key(to_insert[i], i);
    }
    
    vector<KVPair<int32_t, int32_t>> all = lsmTree.C_0[0]->get_all();
    int capacity = num_inserts * 2;
    int numElts = all.size();
    int level = 1;
    auto disklevel = DiskLevel<int32_t,int32_t>(capacity, numElts, level, &all[0]);
    
//    for (int j = 0; j < lsmTree._eltsPerRun; j++){
//        auto kv = all[j];
//        cout << "K: " << kv.key << ", " << "V: " << kv.value << endl;
//    }

}
int main(){

//    runInOrderTest();
//    insertLookupTest();
    diskLevelTest();


    return 0;
    
}
