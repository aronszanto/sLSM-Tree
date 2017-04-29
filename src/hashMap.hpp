//
//  hashMap.hpp
//  lsm-tree
//
//  Created by Aron Szanto on 4/29/17.
//  Copyright © 2017 Aron Szanto. All rights reserved.
//

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


template <typename K, typename V, typename F = KeyHash<K>>
class HashTable {
public:
    HashMap() {
        table = new HashNode<K, V> *[TABLE_SIZE]();
    }
    
    ~HashMap() {
        for (int i = 0; i < TABLE_SIZE; i++) {
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
            entry->setValue(value);
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
    
private:
    HashNode<K, V> **table;
    F hashFunc;
};

#endif /* hashMap_h */
