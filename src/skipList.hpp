//
//  skiplist.hpp
//  lsm-tree
//
//  Created by Aron Szanto on 3/2/17.
//  Copyright © 2017 Aron Szanto. All rights reserved.
//

#ifndef SKIPLIST_H
#define SKIPLIST_H
#include <cstdint>
#include <cstdlib>
#include <random>
#include <string>

#include "run.hpp"

std::default_random_engine generator;
std::uniform_real_distribution<double> distribution(0.0,1.0);
const double NODE_PROBABILITY = 0.5;

namespace lsm{
    
    template<class K,class V, unsigned MAXLEVEL>
    class SkipList_Node {
        
    public:
        const K key;
        V value;
        SkipList_Node<K,V,MAXLEVEL>* _forward[MAXLEVEL+1];
        
        
        SkipList_Node() {
            for (int i=1; i<=MAXLEVEL; i++) {
                _forward[i] = NULL;
            }
        }
        
        SkipList_Node(K searchKey):key(searchKey) {
            for (int i=1; i<=MAXLEVEL; i++) {
                _forward[i] = NULL;
            }
        }
        
        SkipList_Node(K searchKey,V val):key(searchKey),value(val) {
            for (int i=1; i<=MAXLEVEL; i++) {
                _forward[i] = NULL;
            }
        }
        
        virtual ~SkipList_Node(){}
    };
    
    
    template<class K, class V, int MAXLEVEL = 16>
    class SkipList : public Run<K,V>
    {
    public:
        typedef SkipList_Node<K,V,MAXLEVEL> Node;
        const int max_level;
        
        SkipList(K minKey,K maxKey):p_listHead(NULL),p_listTail(NULL),
        cur_max_level(1),max_level(MAXLEVEL),
        _minKey(minKey),_maxKey(maxKey), _n(0)
        {
            p_listHead = new Node(_minKey);
            p_listTail = new Node(_maxKey);
            for (int i=1; i<=MAXLEVEL; i++) {
                p_listHead->_forward[i] = p_listTail;
            }
        }
        
        virtual ~SkipList()
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
        
        void insert_key(const K key,V value) {
//            SkipList_Node<K,V,MAXLEVEL>* update[MAXLEVEL];
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
            }
            
            _n++;
        }
        
        void delete_key(const K searchKey) {
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
        
        V lookup(const K searchKey) {
            Node* currNode = p_listHead;
            for(int level=cur_max_level; level >=1; level--) {
                while (currNode->_forward[level]->key < searchKey) {
                    currNode = currNode->_forward[level];
                }
            }
            currNode = currNode->_forward[1];
            if (currNode->key == searchKey) {
                return currNode->value;
            }
            else {
                return NULL;
            }
        }
        
        bool eltIn(K key) {
            return lookup(key);
        }
        
        inline bool empty() {
            return (p_listHead->_forward[1] == p_listTail);
        }
        
        
        unsigned long long num_elements() {
            return _n;
        }
        
        void set_size(size_t size){
            _maxSize = size;
        }
        
        size_t get_size(){
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
        //        SkipList_Node<K,V,MAXLEVEL>* p_listTail;

    };
}


#endif /* skiplist_h */
