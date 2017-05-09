//
//  skiplist.hpp
//  lsm-tree
//
//    sLSM: Skiplist-Based LSM Tree
//    Copyright © 2017 Aron Szanto. All rights reserved.
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//        You should have received a copy of the GNU General Public License
//        along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#pragma once

#ifndef SKIPLIST_H
#define SKIPLIST_H
#include <cstdint>
#include <cstdlib>
#include <strings.h>
#include <random>
#include <vector>
#include <string>

#include "run.hpp"
using namespace std;

default_random_engine generator;
uniform_real_distribution<double> distribution(0.0,1.0);
const double NODE_PROBABILITY = 0.5;


template<class K,class V, unsigned MAXLEVEL>
class SkipList_Node {
   
    
    
public:
    const K key;
    V value;
    SkipList_Node<K,V,MAXLEVEL>* _forward[MAXLEVEL+1];
    
    
    SkipList_Node(const K searchKey):key(searchKey) {
        for (int i=1; i<=MAXLEVEL; i++) {
            _forward[i] = NULL;
        }
    }
    
    SkipList_Node(const K searchKey,const V val):key(searchKey),value(val) {
        for (int i=1; i<=MAXLEVEL; i++) {
            _forward[i] = NULL;
        }
    }
    
    virtual ~SkipList_Node(){}
};


template<class K, class V, int MAXLEVEL = 12>
class SkipList : public Run<K,V>
{
public:
    
    typedef SkipList_Node<K,V,MAXLEVEL> Node;

    const int max_level;
    K min;
    K max;
    
    SkipList(const K minKey,const K maxKey):p_listHead(NULL),p_listTail(NULL),
    cur_max_level(1),max_level(MAXLEVEL), min((K) NULL), max((K) NULL),
    _minKey(minKey),_maxKey(maxKey), _n(0)
    {
        p_listHead = new Node(_minKey);
        p_listTail = new Node(_maxKey);
        for (int i=1; i<=MAXLEVEL; i++) {
            p_listHead->_forward[i] = p_listTail;
        }
    }
    
    ~SkipList()
    {
        Node* currNode = p_listHead->_forward[1];
        while (currNode != p_listTail) {
            Node* tempNode = currNode;
            currNode = currNode->_forward[1];
            delete tempNode;
        }
        delete p_listHead;
        delete p_listTail;
    }
    
    void insert_key(const K &key, const V &value) {
        if (key > max){
            max = key;
        }
        else if (key < min){
            min = key;
        }
        Node* update[MAXLEVEL];
        Node* currNode = p_listHead;
        for(int level = cur_max_level; level > 0; level--) {
            while (currNode->_forward[level]->key < key) {
                currNode = currNode->_forward[level];
            }
            update[level] = currNode;
        }
        currNode = currNode->_forward[1];
        if (currNode->key == key) {
            // update the value if the key already exists
            currNode->value = value;
        }
        else {
            // if key isn't in the list, insert a new node!
            int insertLevel = generateNodeLevel();
            
            if (insertLevel > cur_max_level && insertLevel < MAXLEVEL - 1) {
                for (int lv = cur_max_level + 1; lv <= insertLevel; lv++) {
                    update[lv] = p_listHead;
                }
                cur_max_level = insertLevel;
            }
            currNode = new Node(key,value);
            for (int level = 1; level <= cur_max_level; level++) {
                currNode->_forward[level] = update[level]->_forward[level];
                update[level]->_forward[level] = currNode;
            }
            ++_n;

        }
        
        
    }
    
    void delete_key(const K &searchKey) {
        //            SkipList_Node<K,V,MAXLEVEL>* update[MAXLEVEL];
        Node* update[MAXLEVEL];
        Node* currNode = p_listHead;
        for(int level=cur_max_level; level >=1; level--) {
            while (currNode->_forward[level]->key < searchKey) {
                currNode = currNode->_forward[level];
            }
            update[level] = currNode;
        }
        currNode = currNode->_forward[1];
        if (currNode->key == searchKey) {
            for (int level = 1; level <= cur_max_level; level++) {
                if (update[level]->_forward[level] != currNode) {
                    break;
                }
                update[level]->_forward[level] = currNode->_forward[level];
            }
            delete currNode;
            // update the max level
            while (cur_max_level > 1 && p_listHead->_forward[cur_max_level] == NULL) {
                cur_max_level--;
            }
        }
        _n--;
    }
    
    V lookup(const K &searchKey, bool &found) {
        Node* currNode = p_listHead;
        for(int level=cur_max_level; level >=1; level--) {
            while (currNode->_forward[level]->key < searchKey) {
                currNode = currNode->_forward[level];
            }
        }
        currNode = currNode->_forward[1];
        if (currNode->key == searchKey) {
            found = true;
            return currNode->value;
        }
        else {
            return (V) NULL;
        }
    }
    
    vector<KVPair<K,V>> get_all(){
        vector<KVPair<K,V>> vec = vector<KVPair<K, V>>();
        auto node = p_listHead->_forward[1];
        while ( node != p_listTail){
            KVPair<K,V> kv = {node->key, node->value};
            vec.push_back(kv);
            // TODO: optimize by reserving space before hand
            node = node->_forward[1];
        }
        return vec;
    }
    
    vector<KVPair<K,V>> get_all_in_range(const K &key1, const K &key2){
        if (key1 > max || key2 < min){
            return (vector<KVPair<K,V>>) {};
        }
        
        vector<KVPair<K,V>> vec = vector<KVPair<K, V>>();
        auto node = p_listHead->_forward[1];
        while ( node->key < key1){
            node = node->_forward[1];
        }

        while ( node->key < key2){
            KVPair<K,V> kv = {node->key, node->value};
            vec.push_back(kv);
            node = node->_forward[1];
        }
        return vec;
        
        
    }

    
    bool eltIn(K &key) {
        return lookup(key);
    }
    
    inline bool empty() {
        return (p_listHead->_forward[1] == p_listTail);
    }
    
    
    unsigned long long num_elements() {
        return _n;
    }
    
    K get_min(){
        return min;
    }
    
    K get_max(){
        return max;
    }
    
    void set_size(unsigned long size){
        _maxSize = size;
    }
    
    size_t get_size_bytes(){
        return _n * (sizeof(K) + sizeof(V));
    }
    
    //    private:
    
    int generateNodeLevel() {
        
        return ffs(rand() & ((1 << MAXLEVEL) - 1)) - 1;
    }
    
    K _minKey;
    K _maxKey;
    unsigned long long _n;
    size_t _maxSize;
    int cur_max_level;
    Node* p_listHead;
    Node* p_listTail;
    uint32_t _keysPerLevel[MAXLEVEL];
    
};



#endif /* skiplist_h */
