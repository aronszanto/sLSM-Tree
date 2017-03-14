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
#include <cstdio>
#include <cstdint>
#include <vector>

namespace lsm {
    
    template <class K, class V>
    class LSM {
        typedef SkipList<K,V> RunType;
        
        
    public:
        std::vector<Run<K,V> *> C_0;
        
        LSM<K,V>(size_t initialSize, size_t runSize, double sizeRatio):_sizeRatio(sizeRatio),_runSize(runSize),_initialSize(initialSize) {
            unsigned long num_runs = initialSize / runSize;
            for (int i = 0; i < num_runs; i++){
                RunType * run = new RunType(INT32_MIN,INT32_MAX);
                run->set_size(runSize);
                C_0.push_back(run);
            }
            _activeRun = 0;
            _eltsPerRun = _runSize / sizeof(K);
        }
        
        void insert_key(K key, V value) {
            
            if (C_0[_activeRun]->num_elements() >= _eltsPerRun)
                ++_activeRun;
            
            // TODO: if (C_0_full) then merge
            
            C_0[_activeRun]->insert_key(key,value);
        }
        
        V lookup(K key){
            for (int i = _activeRun; i >= 0; --i){
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
    private:
        double _sizeRatio;
        size_t _runSize;
        size_t _initialSize;
        unsigned _activeRun;
        unsigned _eltsPerRun;
        
    };
}



#endif /* lsm_h */

