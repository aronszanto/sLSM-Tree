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
#include <cstdio>
#include <cstdint>
#include <vector>

namespace lsm {
    
    template <class K, class V>
    class LSM {
        typedef SkipList<K,V> RunType;
        
        
        public:
        std::vector<Run<K,V>> C_0;
            
            LSM<RunType, K,V>(size_t initialSize, size_t runSize, double sizeRatio):_sizeRatio(sizeRatio),_runSize(runSize),_initialSize(initialSize) {
                unsigned num_runs = initialSize / runSize;
                for (int i = 0; i < num_runs; i++){
                    RunType * run = new RunType();
                    run->set_size(runSize);
                    C_0.push_back(run);
                    
                    
                }
                
            }
            
            // how do you do disk stuff?
        private:
            double _sizeRatio;
            size_t _runSize;
            size_t _initialSize;
        
    };
    
}



#endif /* lsm_h */

