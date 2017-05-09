//
//  main.cpp
//  lsm-tree
//
//    sLSM: Skiplist-Based LSM Tree
//    Copyright © 2017 Aron Szanto. All rights reserved.
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//        You should have received a copy of the GNU General Public License
//        along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdio.h>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <math.h>
#include <random>
#include <algorithm>
#include "skipList.hpp"
#include "bloom.hpp"
#include "hashMap.hpp"
#include "lsm.hpp"


using namespace std;

struct timespec start, finish;
double elapsed;

struct LSMParams {
    const int num_inserts;
    const int num_runs;
    const int elts_per_run;
    const double bf_fp;
    const int pageSize;
    const int disk_runs_per_level;
    const double merge_fraction;
};


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
    clock_gettime(CLOCK_MONOTONIC, &start);    std::cout << "Starting inserts" << std::endl;
    for (int i = 0; i < num_inserts; i++) {
        bf.add(&i, sizeof(i));
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
    double total_insert = (finish.tv_sec - start.tv_sec);
    total_insert += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

    
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
//    std::normal_distribution<double>  distribution(0, 10000000);

    
    const int num_inserts = 1000000;
    const int num_runs = 20;
    const int buffer_capacity = 800;
    const double bf_fp = .001;
    const int pageSize = 512;
    const int disk_runs_per_level = 20;
    const double merge_fraction = 1;
    LSM<int32_t, int32_t> lsmTree = LSM<int32_t, int32_t>(buffer_capacity, num_runs,merge_fraction, bf_fp, pageSize, disk_runs_per_level);
    
    std::vector<int> to_insert;
    for (int i = 0; i < num_inserts; i++) {
        int insert = static_cast<int>(distribution(generator));
        to_insert.push_back(insert);
    }
//    shuffle(to_insert.begin(), to_insert.end(), generator);

    std::cout << "Starting inserts" << std::endl;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < num_inserts; i++) {
        if ( i % 100000 == 0 ) cout << "insert " << i << endl;
        lsmTree.insert_key(to_insert[i],i);
//        lsmTree.printElts();
        
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
    double total_insert = (finish.tv_sec - start.tv_sec);
    total_insert += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    
    std::cout << "Time: " << total_insert << " s" << std::endl;
    std::cout << "Inserts per second: " << (int) num_inserts / total_insert << " s" << std::endl;
    

    std::cout << "Starting lookups" << std::endl;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int lookup;
    for (int i = 0 ; i < num_inserts; i++) {
        if ( i % 100000 == 0 ) cout << "lookup " << i << endl;

        lsmTree.lookup(to_insert[i], lookup);
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
    double total_lookup = (finish.tv_sec - start.tv_sec);
    total_lookup += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    std::cout << "Time: " << total_lookup << " s" << std::endl;
    std::cout << "Lookups per second: " << (int) num_inserts / total_lookup << " s" << std::endl;
}
void runInOrderTest() {
    const int num_inserts = 1000000;
    const int num_runs = 16;
    const int buffer_capacity = 1000;
    const double bf_fp = .2;
    const int pageSize = 4096;
    const int disk_runs_per_level = 10;
    const double merge_fraction = .2;
    LSM<int32_t, int32_t> lsmTree = LSM<int32_t, int32_t>(buffer_capacity, num_runs, merge_fraction, bf_fp, pageSize, disk_runs_per_level);
    

    std::vector<int> to_insert;
    for (int i = 0; i < num_inserts; i++) {
        to_insert.push_back(100 * i);
    }
    std::cout << "Starting inserts" << std::endl;
    clock_gettime(CLOCK_MONOTONIC, &start);
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
    // REDO TEST
//    const int num_inserts = 10000000;
//    const int max_levels = 16;
//    const int num_runs = 10;
//    const int buffer_capacity = 10000;
//    const double bf_fp = .0001;
//    const int pageSize = 4096;
//    const int disk_run_level = 10;
//    LSM<int32_t, int32_t> lsmTree = LSM<int32_t, int32_t>(buffer_capacity, num_runs, 2,.5, bf_fp, pageSize, disk_run_level);
//    
//
//    std::vector<int> to_insert;
//    for (int i = 0; i < num_inserts; i++) {
//        to_insert.push_back(i);
//    }
//    std::clock_t    start_insert;
//    std::cout << "Starting inserts" << std::endl;
//    start_insert = std::clock();
//    for (int i = 0; i < num_inserts; i++) {
//        lsmTree.insert_key(to_insert[i], i);
//    }
//    
//    vector<KVPair<int32_t, int32_t>> all = lsmTree.C_0[0]->get_all();
//    int capacity = num_inserts * 2;
//    int numElts = all.size();
//    int level = 1;
//    auto disklevel = DiskLevel<int32_t,int32_t>(capacity, 4096, level);
//    
}
void customTest(LSMParams &lp, double &ips, double &lps){
    std::random_device                  rand_dev;
    std::mt19937                        generator(rand_dev());
    std::uniform_int_distribution<int>  distribution(INT32_MIN, INT32_MAX);
    
    // unsigned long eltsPerRun, unsigned int numRuns, double merged_frac, double bf_fp, unsigned int pageSize, unsigned int diskRunsPerLevel
    LSM<int32_t, int32_t> lsmTree =
        LSM<int32_t, int32_t>(lp.elts_per_run, lp.num_runs, lp.merge_fraction, lp.bf_fp, lp.pageSize, lp.disk_runs_per_level);
    
    std::vector<int> to_insert;
    for (int i = 0; i < lp.num_inserts; i++) {
        to_insert.push_back(i);
    }
    shuffle(to_insert.begin(), to_insert.end(), generator);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < lp.num_inserts; i++) {
        lsmTree.insert_key(to_insert[i],i);
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
    double total_insert = (finish.tv_sec - start.tv_sec);
    total_insert += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    int lookup;
    for (int i = 0 ; i < lp.num_inserts; i++) {
        
        lsmTree.lookup(to_insert[i], lookup);
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
    double total_lookup = (finish.tv_sec - start.tv_sec);
    total_lookup += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    double ipersec = lp.num_inserts / total_insert;
    double lpersec = lp.num_inserts / total_lookup;
    ips = ipersec;
    lps = lpersec;
    cout << lp.num_inserts << "," << lp.num_runs << "," << lp.elts_per_run << "," << lp.bf_fp << "," << lp.merge_fraction << "," << lp.pageSize << "," << lp.disk_runs_per_level << "," << ipersec << "," << lpersec <<  "," << total_insert << "," << total_lookup << endl;
}
void cartesianTest(){
    cout << "num_inserts,num_runs,elts_per_run,BF_FP,merge_fraction,page_size,disk_runs_per_level,inserts_per_sec,lookups_per_sec,total_insert,total_lookup" << endl;
    vector<int> numins = {10000000};
    vector<int> numruns = {50};
    vector<int> eltspers = {800};
    vector<double> bf_fp = {.001};
    vector<double> merge_frac = {.5, .75, 1.0};
    vector<int> pss = {1024};
    vector<int> drpl = {5, 10, 20};
    auto res = vector<tuple<double, double, LSMParams>>();
    
    for (int i = 0; i < numins.size(); i++)
        for(int n = 0; n < numruns.size(); n++)
            for(int b = 0; b < eltspers.size();b++)
                for(int bf = 0; bf < bf_fp.size(); bf++)
                    for (int m = 0; m < merge_frac.size(); m++)
                        for (int p = 0; p < pss.size(); p++)
                            for (int d = 0; d < drpl.size(); d++){
                // unsigned long eltsPerRun, unsigned int numRuns, double merged_frac, double bf_fp, unsigned int pageSize, unsigned int diskRunsPerLevel
                                // struct LSMParams {
//                                const int num_inserts;
//                                const int num_runs;
//                                const int elts_per_run;
//                                const double bf_fp;
//                                const int pageSize;
//                                const int disk_runs_per_level;
//                                const double merge_fraction;
//                            };
                                LSMParams lp = {numins[i], numruns[n], eltspers[b], bf_fp[bf], pss[p], drpl[d], merge_frac[m]};
                                double ips, lps;
                                customTest(lp, ips, lps);
//                                res.push_back(tuple<double, double, LSMParams>(ips, lps, lp));
                            }
//    sort(res.begin(), res.end());
//    int p10 = ceil(.10 * res.size());
//    res.erase(res.begin() + p10, res.end());
//    ofstream output("binary.data");
//    output.write(static_cast<char *>(&(res[0])), res.size()*sizeof(tuple<double, double, LSMParams>));
    
    
}
//void bfPerfTest(){
//    vector<double> numruns = {.00001, .000001};
//    for (int i = 0; i < numruns.size(); i++)
//        customTest(1000000,	100, 100000, numruns[i],	0.8,1000, 50);
//}
void fencePointerTest(){
//  TODO REDO THIS TEST
//    const long num_inserts = 1000 * 1000 * 1;
//    const int num_lookups = 10000000;
//    const int blocks = 16;
//    const long pageSize = 100;
//    const int num_runs = 10;
//    const long runSize = ceil(num_inserts / num_runs);
//    std::random_device                  rand_dev;
//    std::mt19937                        generator(rand_dev());
//    std::uniform_int_distribution<int>  distribution(0, (int) (num_inserts * 1.2));
//
//    std::vector<KVPair<int32_t, int32_t>> to_insert;
//    auto dl = DiskLevel<int32_t, int32_t>(pageSize, 1, runSize, num_runs, 1);
//    //
//
//    cout << "reserving" << endl;
//    to_insert.reserve(num_inserts);
//    cout << "pushing" << endl;
//    for (int b = 0; b < blocks; b++){
//        for (int i = b * (num_inserts / blocks); i < (b + 1) * num_inserts / blocks; i++) {
//            if (i % 1000000 == 0) cout << "insert " << i << endl;
//
//            to_insert.push_back((KVPair<int32_t, int32_t>) {i, i});
//        }
//        dl.merge(&to_insert[0], num_inserts / blocks);
//        to_insert.resize(0);
//        
//    }
//    
//    auto to_lookup = vector<int>();
//    to_lookup.reserve(num_lookups);
//    for (int i = 0; i< num_lookups; i++){
//        to_lookup.push_back(distribution(generator));
//    }
//    cout << "lookups" << endl;
//    std::clock_t    start_lookup;
//    start_lookup = std::clock();
//    
//    for (int i = 0 ; i < num_lookups; i++) {
//        if (i % 1000000 == 0) cout << "lookup " << i << endl;
//        int lookup = dl.lookup(to_insert[to_lookup[i]].key);
//    }
//    double total_lookup = (std::clock() - start_lookup) / (double)(CLOCKS_PER_SEC);
//    
//    double lpersec = num_lookups / total_lookup;
//    cout << num_inserts << "," << pageSize << "," << lpersec << "," << total_lookup << endl;
//
//    
//    
}

void updateDeleteTest(){
    const int num_inserts = 500;
    const int num_runs = 3;
    const int buffer_capacity = 50;
    const double bf_fp = .01;
    const int pageSize = 1024;
    const int disk_runs_per_level = 2;
    const double merge_fraction = 1;
    LSM<int32_t, int32_t> lsmTree = LSM<int32_t, int32_t>(buffer_capacity, num_runs,merge_fraction, bf_fp, pageSize, disk_runs_per_level);
    
    std::vector<int> to_insert;
    for (int i = 0; i < num_inserts; i++) {
        to_insert.push_back(i);
    }
    
    for (int i = 0; i < num_inserts; i++) {
        lsmTree.insert_key(i, to_insert[i]);
    }
    int lookup;
    for (int i = 0; i < num_inserts; i++) {
        
        lsmTree.lookup(i, lookup);
        assert(to_insert[i] == lookup);
    }
    lsmTree.printStats(); // this is a good demo
    cout << "-----------------------------------------" << endl;
    for (int i = 0; i < num_inserts; i++) {
        to_insert[i] = num_inserts - i;
    }
    
    for (int i = 0; i < num_inserts; i++) {
        lsmTree.insert_key(i, to_insert[i]);
    }
    lsmTree.printStats(); // this is a good demo
    cout << "-----------------------------------------" << endl;
    for (int i = 0; i < num_inserts; i++) {
        lsmTree.lookup(i, lookup);
        assert(to_insert[i] == lookup);
    }

    for (int i = 0; i < num_inserts; i++) {
        lsmTree.delete_key(i);

    }
    lsmTree.printStats(); // this is a good demo
    cout << "-----------------------------------------" << endl;

    int negone = -1;
    for (int i = 0; i < num_inserts * 10; i++) {
        lsmTree.insert_key(i,  negone);
    }
    for (int i = 0; i < num_inserts * 10; i++) {
        
        lsmTree.lookup(i, lookup);
        assert(lookup == -1);
    }
    lsmTree.printStats(); // this is a good demo
    cout << "-----------------------------------------" << endl;



    
}
void rangeTimeTest(){
    const int num_inserts = 50000000;
    const int num_runs = 20;
    const int buffer_capacity = 500;
    const double bf_fp = .01;
    const int pageSize = 512;
    const int disk_runs_per_level = 5;
    const double merge_fraction = 1;
    LSM<int32_t, int32_t> lsmTree = LSM<int32_t, int32_t>(buffer_capacity, num_runs,merge_fraction, bf_fp, pageSize, disk_runs_per_level);
    
    std::vector<int> to_insert;
    
    for (int i = 0; i < num_inserts; i++) {
        to_insert.push_back(i);
    }
    shuffle(to_insert.begin(), to_insert.end(), generator);
    
    for (int i = 0; i < num_inserts; i++) {
        lsmTree.insert_key(to_insert[i], i);
    }
    cout << "range_size time" << endl;
    for (int i = 20; i < 20000001; i *= 10){
        
        int n1 = -i;
        int n2 = i;
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        lsmTree.range(n1, n2);
        clock_gettime(CLOCK_MONOTONIC, &finish);
        double total_range= (finish.tv_sec - start.tv_sec);
        total_range += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
        cout << i << " " << total_range << endl;
        
    }
}
void rangeTest(){
    const int num_inserts = 10000000;
    const int num_runs = 20;
    const int buffer_capacity = 500;
    const double bf_fp = .01;
    const int pageSize = 512;
    const int disk_runs_per_level = 5;
    const double merge_fraction = 1;
    LSM<int32_t, int32_t> lsmTree = LSM<int32_t, int32_t>(buffer_capacity, num_runs,merge_fraction, bf_fp, pageSize, disk_runs_per_level);

    std::vector<int> to_insert;
    
    for (int i = 0; i < num_inserts; i++) {
        to_insert.push_back(i);
    }
    shuffle(to_insert.begin(), to_insert.end(), generator);

    for (int i = 0; i < num_inserts; i++) {
        lsmTree.insert_key(to_insert[i], i);
    }
    
    int n1 = 0;
    int n2 = 5000000;
    auto r = lsmTree.range(n1, n2);
    assert(r.size() == (n2 - n1));
    int negone = -1;
    for (int i = n1; i < n2; i++) {
        lsmTree.insert_key(i, negone);
    }
    r = lsmTree.range(n1, n2);
    assert(r.size() == (n2 - n1));
    int nd = 2000000;

    for (int i = n1; i < n1 + nd; i++) {
        lsmTree.delete_key(i);

    }

    r = lsmTree.range(n1, n2);
    assert(r.size() == (n2 - n1 - nd));
//    lsmTree.printElts();
}

void concurrentLookupTest(){
    std::random_device                  rand_dev;
    std::mt19937                        generator(rand_dev());
    
    
    
    const int num_inserts = 3000000;
    const int num_lookups = 1000000;
    const int num_runs = 50;
    const int buffer_capacity = 800;
    const double bf_fp = .001;
    const int pageSize = 512;
    const int disk_runs_per_level = 10;
    const double merge_fraction = 1;
    cout << "iv ips" << endl;
    for (double d = 10; d < 1000000000; d *= 100){
        std::normal_distribution<double>  distribution1(0, d);
        auto lsmTree = LSM<int32_t, int32_t>(buffer_capacity, num_runs,merge_fraction, bf_fp, pageSize, disk_runs_per_level);
        
        std::vector<int> to_insert;
        vector<int> to_lookup;
        for (int i = 0; i < num_inserts; i++) {
            int insert = (int) distribution1(generator);
            to_insert.push_back(insert);
        }
        
       
    //    shuffle(to_insert.begin(), to_insert.end(), generator);
        
    //    std::cout << "Starting inserts" << std::endl;
        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int i = 0; i < num_inserts; i++) {
    //        if ( i % 100000 == 0 ) cout << "insert " << i << endl;
            lsmTree.insert_key(to_insert[i],i);
            
        }
        clock_gettime(CLOCK_MONOTONIC, &finish);
        double total_insert = (finish.tv_sec - start.tv_sec);
        total_insert += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    //    std::cout << "Time: " << total_insert << " s" << std::endl;
    //    std::cout << "Inserts per second: " << (int) num_inserts / total_insert << " s" << std::endl;
        
        cout << d << " " << (int) num_inserts / total_insert << endl;
    }
    auto lsmTree = LSM<int32_t, int32_t>(buffer_capacity, num_runs,merge_fraction, bf_fp, pageSize, disk_runs_per_level);
    std::uniform_int_distribution<int>  distribution(INT_MIN, INT_MAX);

    std::vector<int> to_insert;
    vector<int> to_lookup;
    for (int i = 0; i < num_inserts; i++) {
        int insert_late = (int) distribution(generator);
        to_insert.push_back(insert_late);
    }
    for (int i = 0; i < num_inserts; i++) {
        lsmTree.insert_key(to_insert[i],i);
        
    }
    
    
    sleep(2);
//    std::cout << "Starting lookups" << std::endl;
//    int nthreads = nt;
    cout << "variance nthreads time lookups/sec" << endl;
    for (double lv = 10; lv < num_lookups * 2; lv *= 1000){
        std::normal_distribution<double>  distribution2(0, lv);
        
        for (int i = 0; i < num_lookups; i++) {
            int lookup = (int) distribution2(generator);
            to_lookup.push_back(lookup);
        }
        
        for (int i = 1; i <= 3; i += 1){

            struct timespec start, finish;
            
            clock_gettime(CLOCK_MONOTONIC, &start);
            int nthreads = i;

            auto threads = vector<thread>(nthreads);
            
            
            for (int t = 0; t < nthreads; t++){
                threads[t] = thread ([&] {
                    unsigned m = rand();
                    int lookup;
                    for (int i = 0 ; i < num_lookups; i++) {
                        //                cout << (1737119 * m * i) % to_insert.size() << endl;
                        lsmTree.lookup(to_lookup[(1737119 * m * i) % to_lookup.size()], lookup);
                    }
                    
                });
            }
            for (int t = 0; t < nthreads; t++)
                threads[t].join();
            
            
            
            clock_gettime(CLOCK_MONOTONIC, &finish);
            
            double total_lookup = (finish.tv_sec - start.tv_sec);
            total_lookup += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
           
    //        cout << "Number of Threads: " << nthreads << endl;
        //    std::cout << "Time: " << total_lookup << " s" << std::endl;
    //        std::cout << "Lookups per second: " << (int) nthreads * num_lookups / total_lookup << " s" << std::endl;
            std::cout << lv << " " << nthreads << " " << total_lookup << " " << (int) nthreads * num_lookups / total_lookup << endl;
        }
    }
}

void tailLatencyTest(){
    std::random_device                  rand_dev;
    std::mt19937                        generator(rand_dev());
    std::uniform_int_distribution<int>  distribution(INT32_MIN, INT32_MAX);
    
    
    const int num_inserts = 10000000;
    const int num_runs = 200;
    const int buffer_capacity = 2000;
    const double bf_fp = .001;
    const int pageSize = 512;
    const int disk_runs_per_level = 2;
    const double merge_fraction = 1;
    LSM<int32_t, int32_t> lsmTree = LSM<int32_t, int32_t>(buffer_capacity, num_runs,merge_fraction, bf_fp, pageSize, disk_runs_per_level);
    
    std::vector<int> to_insert;
    for (int i = 0; i < num_inserts; i++) {
        //        int insert = distribution(generator);
        to_insert.push_back(i);
    }
    shuffle(to_insert.begin(), to_insert.end(), generator);
    
    auto times = vector<double>(num_inserts);
    
    //    std::cout << "Starting inserts" << std::endl;
    
    for (int i = 0; i < num_inserts; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        lsmTree.insert_key(to_insert[i],i);
        clock_gettime(CLOCK_MONOTONIC, &finish);
        times.push_back((finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1000000000.);
    }
    sort(times.begin(), times.end());
    cout << "largest latency: " << times[times.size() - 1] << endl;
    cout << "smallest latency: " << times[0] << endl;

}

void hardCodeTest(int num_inserts, int num_runs, int elts_per_run, double bf_fp, double merge_fraction, int pageSize, int disk_runs_per_level){
        std::random_device                  rand_dev;
        std::mt19937                        generator(rand_dev());
        std::uniform_int_distribution<int>  distribution(INT32_MIN, INT32_MAX);
        
        // unsigned long eltsPerRun, unsigned int numRuns, double merged_frac, double bf_fp, unsigned int pageSize, unsigned int diskRunsPerLevel
        LSM<int32_t, int32_t> lsmTree =
        LSM<int32_t, int32_t>(elts_per_run, num_runs, merge_fraction, bf_fp, pageSize, disk_runs_per_level);
        
        std::vector<int> to_insert;
        for (int i = 0; i < num_inserts; i++) {
            to_insert.push_back(i);
        }
        shuffle(to_insert.begin(), to_insert.end(), generator);
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < num_inserts; i++) {
            lsmTree.insert_key(to_insert[i],i);
        }
        clock_gettime(CLOCK_MONOTONIC, &finish);
        double total_insert = (finish.tv_sec - start.tv_sec);
        total_insert += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        int lookup;
        for (int i = 0 ; i < num_inserts; i++) {

            lsmTree.lookup(to_insert[i], lookup);
        }
        clock_gettime(CLOCK_MONOTONIC, &finish);
        double total_lookup = (finish.tv_sec - start.tv_sec);
        total_lookup += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
        double ipersec = num_inserts / total_insert;
        double lpersec = num_inserts / total_lookup;
        cout << num_inserts << "," << num_runs << "," << elts_per_run << "," << bf_fp << "," << merge_fraction << "," << pageSize << "," << disk_runs_per_level << "," << ipersec << "," << lpersec <<  "," << total_insert << "," << total_lookup << endl;
}

void updateLookupSkewTest(){
    std::random_device                  rand_dev;
    std::mt19937                        generator(rand_dev());
    
    
    
    const int num_total = 1000000;
    const int num_runs = 80;
    const int buffer_capacity = 800;
    const double bf_fp = .001;
    const int pageSize = 512;
    const int disk_runs_per_level = 20;
    const double merge_fraction = 1;
    
    cout << "lookup_pct total_time" << endl;
    for (double i = .01; i < .95; i+=.1){
        LSM<int32_t, int32_t> lsmTree = LSM<int32_t, int32_t>(buffer_capacity, num_runs,merge_fraction, bf_fp, pageSize, disk_runs_per_level);

        std::uniform_int_distribution<int>  distribution(0, INT_MAX);
        std::vector<int> to_query;
        for (int j = 0; j < num_total; j++) {
            
            int num = (int) distribution(generator);
            to_query.push_back(num);
        }
        int lookup;
        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int j = 0; j < num_total; ++j){
            if (to_query[j] < (int) floor(i * INT_MAX)){
                
                lsmTree.lookup(to_query[j], lookup);
//                cout << "lookup " << to_query[j] << endl;
            }
            else {
                lsmTree.insert_key(to_query[j], j);
//                cout << "insert " << to_query[j] << endl;
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &finish);
        double total = (finish.tv_sec - start.tv_sec);
        total += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

        cout << i << " " << total << endl;
    }


}

void loadFromBin(LSM<int, int> &lsm, string filename){
    FILE *intArrayFile;
    long size;
    
    
    intArrayFile = fopen(filename.c_str(), "rb");
    fseek(intArrayFile, 0, SEEK_END);
    size = ftell(intArrayFile);
    
    int new_array[size / sizeof(int)];
    
    rewind(intArrayFile);
    size_t num;
    num = fread(new_array, sizeof(int), size/sizeof(int) + 1, intArrayFile);
    assert(num == size / sizeof(int));
    
    int *ptr = new_array;
    int read = 0;
    int k,v;
    while (read + 1 < num){
        k = *ptr;
        v = *(ptr + 1);
        lsm.insert_key(k, v);
        ptr += 2;
        read += 2;
    }
}


void queryLine(LSM<int, int> &lsm, const string &line, vector<string> &strings){
    unsigned long pos = line.find(' ');
    unsigned long ip = 0;
    strings.clear();
    
    // Decompose statement
    while( pos != string::npos ) {
        strings.push_back( line.substr( ip, pos - ip + 1 ) );
        ip = pos + 1;
        
        pos = line.find( ' ', ip );
    }
    
    // Add the last one
    strings.push_back( line.substr( ip, (pos < line.size() ? pos : line.size()) - ip + 1 ) );
    
    switch ((char) strings[0].c_str()[0]){
        case 'p':{
            int pk = stoi(strings[1]);
            int v = stoi(strings[2]);
            lsm.insert_key(pk, v);
        }
            break;
        case 'g': {
            int lk = stoi(strings[1]);
            int v;
            bool found = lsm.lookup(lk, v);
            if (found) {
                cout << v;
            }
            
            cout << endl;
        }
            break;
        case 'r':{
            int lk1 = stoi(strings[1]);
            int lk2 = stoi(strings[2]);
            auto res = lsm.range(lk1, lk2);
            if (!res.empty()){
                for (int i = 0; i < res.size(); ++i){
                    cout << res[i].key << ":" << res[i].value << " ";
                }
            }
            cout << endl;

        }
            break;
        case 'd': {
            int dk = stoi(strings[1]);
            lsm.delete_key(dk);
        }
            break;
        case 'l': {
            string ls = strings[1];
            loadFromBin(lsm, ls);
        }
            break;
        case 's': {
            lsm.printStats();
        }
            

    }

}
int main(int argc, char *argv[]){

//    insertLookupTest();
//    updateDeleteTest();
//    rangeTest();
//    rangeTimeTest();
//    concurrentLookupTest();
//    tailLatencyTest();
//    cartesianTest();
//    updateLookupSkewTest();
    
    auto lsm = LSM<int, int>(800,20,1.0,0.00100,1024,20);
    auto strings = vector<string>(3);
    if (argc == 2){
    cout << "LSM Tree DSL Interactive Mode" << endl;
        while (true){
            cout << "> ";
            string input;
            getline(cin, input);
            queryLine(lsm, input, strings);
        }
    }
    else{
        string line;
        ifstream f;
        for (int i = 1; i < argc; ++i){
            f.open(argv[i]);
            
            if(!f.is_open()) {
                perror("Error open");
                exit(EXIT_FAILURE);
            }
            while(getline(f, line)) {
                queryLine(lsm, line, strings);
            }
        }
    }







    
    
    
    return 0;
    
}
