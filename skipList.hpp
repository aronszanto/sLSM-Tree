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
        K key;
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
        
        SkipList(K minKey,K maxKey):m_pHeader(NULL),m_pTail(NULL),
        max_curr_level(1),max_level(MAXLEVEL),
        m_minKey(minKey),m_maxKey(maxKey)
        {
            m_pHeader = new Node(m_minKey);
            m_pTail = new Node(m_maxKey);
            for (int i=1; i<=MAXLEVEL; i++) {
                m_pHeader->_forward[i] = m_pTail;
            }
        }
        
        virtual ~SkipList()
        {
            Node* currNode = m_pHeader->_forward[1];
            while (currNode != m_pTail) {
                Node* tempNode = currNode;
                currNode = currNode->_forward[1];
                delete tempNode;
            }
            delete m_pHeader;
            delete m_pTail;
        }
        
        void insert_key(const K key,V value)
        {
//            SkipList_Node<K,V,MAXLEVEL>* update[MAXLEVEL];
            Node* update[MAXLEVEL];
            Node* currNode = m_pHeader;
            for(int level=max_curr_level; level > 0; level--) {
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
                if (insertLevel > max_curr_level) {
                    for (int level = max_curr_level + 1; level <= insertLevel; level++) {
                        update[level] = m_pHeader;
                    }
                    max_curr_level = insertLevel;
                }
                currNode = new Node(key,value);
                for (int lv=1; lv<=max_curr_level; lv++) {
                    currNode->_forward[lv] = update[lv]->_forward[lv];
                    update[lv]->_forward[lv] = currNode;
                }
            }
        }
        
        void delete_key(const K searchKey)
        {
//            SkipList_Node<K,V,MAXLEVEL>* update[MAXLEVEL];
            Node* update[MAXLEVEL];
            Node* currNode = m_pHeader;
            for(int level=max_curr_level; level >=1; level--) {
                while (currNode->_forward[level]->key < searchKey) {
                    currNode = currNode->_forward[level];
                }
                update[level] = currNode;
            }
            currNode = currNode->_forward[1];
            if (currNode->key == searchKey) {
                for (int lv = 1; lv <= max_curr_level; lv++) {
                    if (update[lv]->_forward[lv] != currNode) {
                        break;
                    }
                    update[lv]->_forward[lv] = currNode->_forward[lv];
                }
                delete currNode;
                // update the max level
                while (max_curr_level > 1 && m_pHeader->_forward[max_curr_level] == NULL) {
                    max_curr_level--;
                }
            }
        }
        
        V lookup(const K searchKey)
        {
            Node* currNode = m_pHeader;
            for(int level=max_curr_level; level >=1; level--) {
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
        
        bool empty() const
        {
            return (m_pHeader->_forward[1] == m_pTail);
        }
        
        
        
    private:
        
        int generateNodeLevel() {
            int level = 1;
            
            while (distribution(generator) < NODE_PROBABILITY && level < MAXLEVEL) {
                level++;
            }
            return level;
        }
        K m_minKey;
        K m_maxKey;
        int max_curr_level;
        Node* m_pHeader;
        Node* m_pTail;
        //        SkipList_Node<K,V,MAXLEVEL>* m_pTail;

    };
}


#endif /* skiplist_h */
