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
#include <array>
#include <math.h>

#include "MurmurHash.h"

using namespace std;

template<class Key, class Hash = hash<Key>>
class BloomFilter {
public:
    BloomFilter(uint64_t n, double fp) {
        
        double denom = 0.480453013918201; // (ln(2))^2

        double size = -1 * (double) n * (log(fp) / denom);
        
        m_bits = vector<bool>((int) size);
        
        double ln2 = 0.693147180559945;
        m_numHashes = (int) ceil( (size / n) * ln2);  // ln(2)
    }
    
    array<uint64_t, 2> hash(const Key *data, size_t len) {
        
        array<uint64_t, 2> hashValue;
        
        MurmurHash3_x64_128(data, len, 0, hashValue.data());
        
        return hashValue;
    }
    
    uint64_t nthHash(uint8_t n, uint64_t hashA, uint64_t hashB, uint64_t filterSize) {
        return (hashA + n * hashB) % filterSize;
    }
    
    void add(const Key *data, size_t len) {
        auto hashValues = hash(data, len);
        
        for (int n = 0; n < m_numHashes; n++) {
            m_bits[nthHash(n, hashValues[0], hashValues[1], m_bits.size())] = true;
        }
    }
    
    bool mayContain(const Key *data, size_t len) {
        auto hashValues = hash(data, len);
        
        for (int n = 0; n < m_numHashes; n++) {
            if (!m_bits[nthHash(n, hashValues[0], hashValues[1], m_bits.size())]) {
                return false;
            }
        }
        
        return true;
    }
    
private:
    uint8_t m_numHashes;
    vector<bool> m_bits;
};




#endif /* bloom_h */
