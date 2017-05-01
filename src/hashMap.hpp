#pragma once

//
//  hashMap.hpp
//  lsm-tree
//
//  Created by Aron Szanto on 4/29/17.
//  Copyright © 2017 Aron Szanto. All rights reserved.
//
#include "MurmurHash.h"

#ifndef hashMap_h
#define hashMap_h


template <typename K, typename V>
class HashTable {
public:
    unsigned long _size;
    unsigned long _elts;
    KVPair<K,V> EMPTY = {INT_MIN, INT_MIN};
    
    HashTable(unsigned long size): _size(2 * size), _elts(0) {
        table = new KVPair<K, V> [_size]();
        fill(table + 0, table + _size, (KVPair<K,V>) EMPTY);
    }
    
    ~HashTable() {
//        for (int i = 0; i < _size; i++) {
//            delete table[i];
//        }
        delete [] table;
    }
    
    void resize(){
        _size *= 2;
        auto newTable = new KVPair<K,V> [_size]();
        fill(newTable + 0, newTable + _size, (KVPair<K,V>) EMPTY);

        for (unsigned long i = 0; i < _size / 2; i++){
            if (table[i] != EMPTY){
                unsigned long newHash = hashFunc(table[i].key);
                
                for (int j = 0;; j++){
                    if (newTable[(newHash + j) % _size] == EMPTY){
                        newTable[(newHash + j) % _size] = table[i];
                        break;
                    }
                }

            }
        }
        delete [] table;
        
        table = newTable;
        
            
        
    }
    
    bool get(const K &key, V &value) {
        unsigned long hashValue = hashFunc(key);
        for (int i = 0;; ++i){
            if (table[(hashValue + i) % _size] == EMPTY){
                return false;
            }
            else if (table[(hashValue + i) % _size].key == key){
                value = table[(hashValue + i) % _size].value;
                return true;
            }
        }

        return false;
    }
    
    void put(const K &key, const V &value) {
        if (_elts * 2 > _size){
            resize();
        }
        unsigned long hashValue = hashFunc(key);
        KVPair<K, V> node;
        
        for (unsigned long i = 0;; i++){
            if (table[(hashValue + i) % _size] == EMPTY){
                table[(hashValue + i) % _size].key = key;
                table[(hashValue + i) % _size].value = value;
                ++_elts;
                return;
            }
            else if (table[(hashValue + i) % _size].key == key){
                
                table[(hashValue + i) % _size].value = value;
                return;
            }
        }
    }
    
    V putIfEmpty(const K &key, const V &value) {
        if (_elts * 2 > _size){
            resize();
        }
        unsigned long hashValue = hashFunc(key);
        
        for (unsigned long i = 0;; i++){
            if (table[(hashValue + i) % _size] == EMPTY){
                table[(hashValue + i) % _size].key = key;
                table[(hashValue + i) % _size].value = value;
                ++_elts;
                return NULL;
            }
            else if (table[(hashValue + i) % _size].key == key){
                // something already here, return current occupant to user
                return table[(hashValue + i) % _size].value;
            }
        }
    }
    
    
    unsigned long hashFunc(const K key){
        array<unsigned long, 2> hashValue;
        
        MurmurHash3_x64_128(&key, sizeof(K), 0, hashValue.data());
        return  (hashValue[0] % _size);
    }
    
private:
    KVPair<K, V> *table;
};

#endif /* hashMap_h */
