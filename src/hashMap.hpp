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
struct HashNode
{
    K key;
    V value;
    
    HashNode(const K &key, const V &value) :
    key(key), value(value)
    {
    }
};


template <typename K, typename V>
class HashTable {
public:
    int _size;
    HashTable(int size): _size(size) {
        table = new HashNode<K, V> *[size]();
    }
    
    ~HashTable() {
        for (int i = 0; i < _size; i++) {
            delete table[i];
        }
        delete [] table;
    }
    
    bool get(const K &key, V &value) {
        unsigned long hashValue = hashFunc(key);
        auto entry = table[hashValue];
        
            if (key == entry->key) {
                value = entry->value;
                return true;
            }

        return false;
    }
    
    void put(const K &key, const V &value) {
        unsigned long hashValue = hashFunc(key);
        HashNode<K, V> *entry = table[hashValue];
        
        if (entry == NULL) {
            entry = new HashNode<K, V>(key, value);
            table[hashValue] = entry;
        }
        else {
            entry->value = value;
        }
    }
    
    void remove(const K &key) {
        unsigned long hashValue = hashFunc(key);
        HashNode<K, V> *entry = table[hashValue];
        
        
        if (!entry) {
            return;
        }
        else {
            delete entry;
        }
    }
    
    int hashFunc(const K &key){
        int res;
        MurmurHash3_x86_32(&key, sizeof(K), 0, &res);
        return (res % _size);
    }
    
private:
    HashNode<K, V> **table;
};

#endif /* hashMap_h */
