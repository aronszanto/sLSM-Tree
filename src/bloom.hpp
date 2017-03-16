//
//  bloom.hpp
//  lsm-tree
//
//  Created by Aron Szanto on 3/14/17.
//  Copyright © 2017 Aron Szanto. All rights reserved.
//

#ifndef bloom_h
#define bloom_h

#include <stdio.h>
#include <cstdint>
#include <vector>
#include "MurmurHash.h"

using namespace std;
namespace lsm {
    
    template<class Key, class Hash = hash<Key>>
    class BloomFilter {
    public:
        BloomFilter(uint64_t size, uint8_t numHashes)
        : m_bits(size),
        m_numHashes(numHashes) {}
        
        void add(const uint8_t *data, size_t len);
        bool possiblyContains(const Key *data, size_t len) const;
        
    private:
        uint8_t m_numHashes;
        vector<bool> m_bits;
    };
    
    
}


#endif /* bloom_h */
