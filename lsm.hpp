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
        
        
    public:
        Run<K,V> * C_0;
    };
    
}



#endif /* lsm_h */

