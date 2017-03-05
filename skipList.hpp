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
#include <string>
#include "run.hpp"

namespace lsm{
    
    template<class K,class V, unsigned MAXLEVEL>
    class SkipList_Node {
    public:
        K key;
        V value;
        SkipList_Node<K,V,MAXLEVEL>* forwards[MAXLEVEL+1];
        
        
        SkipList_Node() {
            for ( int i=1; i<=MAXLEVEL; i++ ) {
                forwards[i] = NULL;
            }
        }
        
        SkipList_Node(K searchKey):key(searchKey) {
            for ( int i=1; i<=MAXLEVEL; i++ ) {
                forwards[i] = NULL;
            }
        }
        
        SkipList_Node(K searchKey,V val):key(searchKey),value(val) {
            for ( int i=1; i<=MAXLEVEL; i++ ) {
                forwards[i] = NULL;
            }
        }
        
        virtual ~SkipList_Node(){}
    };
    
    
    template<class K, class V, int MAXLEVEL = 16>
    class SkipList : public Run<K,V>
    {
    public:
        typedef K KeyType;
        typedef V ValueType;
        typedef SkipList_Node<K,V,MAXLEVEL> NodeType;
        
        SkipList(K minKey,K maxKey):m_pHeader(NULL),m_pTail(NULL),
        max_curr_level(1),max_level(MAXLEVEL),
        m_minKey(minKey),m_maxKey(maxKey)
        {
            m_pHeader = new NodeType(m_minKey);
            m_pTail = new NodeType(m_maxKey);
            for ( int i=1; i<=MAXLEVEL; i++ ) {
                m_pHeader->forwards[i] = m_pTail;
            }
        }
        
        virtual ~SkipList()
        {
            NodeType* currNode = m_pHeader->forwards[1];
            while ( currNode != m_pTail ) {
                NodeType* tempNode = currNode;
                currNode = currNode->forwards[1];
                delete tempNode;
            }
            delete m_pHeader;
            delete m_pTail;
        }
        
        void insert_key(K searchKey,V newValue)
        {
            SkipList_Node<K,V,MAXLEVEL>* update[MAXLEVEL];
            NodeType* currNode = m_pHeader;
            for(int level=max_curr_level; level >=1; level--) {
                while ( currNode->forwards[level]->key < searchKey ) {
                    currNode = currNode->forwards[level];
                }
                update[level] = currNode;
            }
            currNode = currNode->forwards[1];
            if ( currNode->key == searchKey ) {
                currNode->value = newValue;
            }
            else {
                int newlevel = randomLevel();
                if ( newlevel > max_curr_level ) {
                    for ( int level = max_curr_level+1; level <= newlevel; level++ ) {
                        update[level] = m_pHeader;
                    }
                    max_curr_level = newlevel;
                }
                currNode = new NodeType(searchKey,newValue);
                for ( int lv=1; lv<=max_curr_level; lv++ ) {
                    currNode->forwards[lv] = update[lv]->forwards[lv];
                    update[lv]->forwards[lv] = currNode;
                }
            }
        }
        
        void delete_key(K searchKey)
        {
            SkipList_Node<K,V,MAXLEVEL>* update[MAXLEVEL];
            NodeType* currNode = m_pHeader;
            for(int level=max_curr_level; level >=1; level--) {
                while ( currNode->forwards[level]->key < searchKey ) {
                    currNode = currNode->forwards[level];
                }
                update[level] = currNode;
            }
            currNode = currNode->forwards[1];
            if ( currNode->key == searchKey ) {
                for ( int lv = 1; lv <= max_curr_level; lv++ ) {
                    if ( update[lv]->forwards[lv] != currNode ) {
                        break;
                    }
                    update[lv]->forwards[lv] = currNode->forwards[lv];
                }
                delete currNode;
                // update the max level
                while ( max_curr_level > 1 && m_pHeader->forwards[max_curr_level] == NULL ) {
                    max_curr_level--;
                }
            }
        }
        
        V lookup(K searchKey)
        {
            NodeType* currNode = m_pHeader;
            for(int level=max_curr_level; level >=1; level--) {
                while ( currNode->forwards[level]->key < searchKey ) {
                    currNode = currNode->forwards[level];
                }
            }
            currNode = currNode->forwards[1];
            if ( currNode->key == searchKey ) {
                return currNode->value;
            }
            else {
                return NULL;
            }
        }
        
        bool empty() const
        {
            return ( m_pHeader->forwards[1] == m_pTail );
        }
        
        const int max_level;
        
    protected:
        double uniformRandom()
        {
            return rand() / double(RAND_MAX);
        }
        
        int randomLevel() {
            int level = 1;
            double p = 0.5;
            while ( uniformRandom() < p && level < MAXLEVEL ) {
                level++;
            }
            return level;
        }
        K m_minKey;
        K m_maxKey;
        int max_curr_level;
        SkipList_Node<K,V,MAXLEVEL>* m_pHeader;
        SkipList_Node<K,V,MAXLEVEL>* m_pTail;
    };
}


#endif /* skiplist_h */
