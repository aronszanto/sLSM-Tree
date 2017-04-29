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
    unsigned long _size;
    unsigned long _elts;
    HashTable(unsigned long size): _size(2 * size), _elts(0) {
        table = new HashNode<K, V> *[_size]();
    }
    
    ~HashTable() {
        for (int i = 0; i < _size; i++) {
            delete table[i];
        }
        delete [] table;
    }
    
    void resize(){
        _size *= 2;
        auto newTable = new HashNode<K,V> *[_size]();
        for (unsigned long i = 0; i < _size / 2; i++){
            auto oldNode = table[i];
            if (oldNode){
                unsigned long newHash = hashFunc(oldNode->key);
                HashNode<K, V> *newNode;
                
                for (int i = 0;; i++){
                    newNode = newTable[newHash + i];
                    if (!newNode){
                        newTable[newHash + i] = oldNode;
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
            if (!table[hashValue + i]){
                return false;
            }
            else if (table[hashValue + i]->key == key){
                value = table[hashValue + i]->value;
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
        HashNode<K, V> *node;
        
        for (unsigned long i = 0;; i++){
            node = table[hashValue + i];
            if (!node){
                node = new HashNode<K, V>(key, value);
                table[hashValue + i] = node;
                ++_elts;
                return;
            }
            else if (node->key == key){
                
                node->value = value;
                return;
            }
        }
    }
    
    
    unsigned long hashFunc(const K key){
        array<unsigned long, 2> hashValue;
        
        MurmurHash3_x64_128(&key, sizeof(K), 0, hashValue.data());
        return  (hashValue[0] % _size);
    }
    
private:
    HashNode<K, V> **table;
};

#endif /* hashMap_h */
