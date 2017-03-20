//
//  lsm.hpp
//  lsm-tree
//
//  Created by Aron Szanto on 3/3/17.
//  Copyright © 2017 Aron Szanto. All rights reserved.
//

#ifndef LSM_H
#define LSM_H

#include "run.hpp"
#include "skipList.hpp"
#include "bloom.hpp"
#include <cstdio>
#include <cstdint>
#include <vector>

const double BF_FP_RATE = .05;


template <class K, class V>
class LSM {
    
    typedef SkipList<K,V> RunType;
    
    typedef struct DiskLevel {
        
    } DiskLevel;
    
    
public:
    std::vector<Run<K,V> *> C_0;
    vector<BloomFilter<K> *> filters;
    
    LSM<K,V>(size_t initialSize, size_t runSize, double sizeRatio):_sizeRatio(sizeRatio),_runSize(runSize),_initialSize(initialSize) {
        _activeRun = 0;
        _eltsPerRun = _runSize / sizeof(K);
        _bfFalsePositiveRate = BF_FP_RATE;
        
        unsigned long num_runs = initialSize / runSize;
        for (int i = 0; i < num_runs; i++){
            RunType * run = new RunType(INT32_MIN,INT32_MAX);
            run->set_size(runSize);
            C_0.push_back(run);
            
            BloomFilter<K> * bf = new BloomFilter<K>(_eltsPerRun, _bfFalsePositiveRate);
            filters.push_back(bf);
        }
        
    }
    
    void insert_key(K key, V value) {
        
        if (C_0[_activeRun]->num_elements() >= _eltsPerRun)
            ++_activeRun;
        
        // TODO: if (C_0_full) then merge
//        cout << "adding key " << key<< " to run and filter " << _activeRun << endl;

        C_0[_activeRun]->insert_key(key,value);
        filters[_activeRun]->add(&key, sizeof(K));
    }
    
    V lookup(K key){
        // TODO keep track of min/max in runs?
//        cout << "looking for key " << key << endl;
        for (int i = _activeRun; i >= 0; --i){
//            cout << "... in run/filter " << i << endl;
            if (!filters[i]->mayContain(&key, sizeof(K)))
                continue;
            
            V lookupRes = C_0[i]->lookup(key);
            if (lookupRes)
                return lookupRes;
        }
        return NULL;
    }
    
    
    unsigned long long num_elements(){
        unsigned long long total = 0;
        for (int i = 0; i <= _activeRun; ++i)
            total += C_0[i]->num_elements();
        return total;
    }
    
    // how do you do disk stuff?
//private: // TODO MAKE PRIVATE
    double _sizeRatio;
    size_t _runSize;
    size_t _initialSize;
    unsigned _activeRun;
    unsigned _eltsPerRun;
    double _bfFalsePositiveRate;
    
};




#endif /* lsm_h */

