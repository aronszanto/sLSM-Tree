//
//  diskLevel.hpp
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

#ifndef diskLevel_h
#define diskLevel_h
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include "run.hpp"
#include "diskRun.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cassert>
#include <algorithm>

#define LEFTCHILD(x) 2 * x + 1
#define RIGHTCHILD(x) 2 * x + 2
#define PARENT(x) (x - 1) / 2

int TOMBSTONE = INT_MIN;

using namespace std;


template <class K, class V>
class DiskLevel {
    
public: // TODO make some of these private
    typedef KVPair<K,V> KVPair_t;
    typedef pair<KVPair<K, V>, int> KVIntPair_t;
    KVPair_t KVPAIRMAX;
    KVIntPair_t KVINTPAIRMAX;
    V V_TOMBSTONE = (V) TOMBSTONE;

    struct StaticHeap {
        int size ;
        vector<KVIntPair_t> arr;
        KVIntPair_t max;
        
        StaticHeap(unsigned sz, KVIntPair_t mx) {
            size = 0;
            arr = vector<KVIntPair_t>(sz, mx);
            max = mx;
        }
        
        void push(KVIntPair_t blob) {
            unsigned i = size++;
            while(i && blob < arr[PARENT(i)]) {
                arr[i] = arr[PARENT(i)] ;
                i = PARENT(i) ;
            }
            arr[i] = blob ;
        }
        void heapify(int i) {
            int smallest = (LEFTCHILD(i) < size && arr[LEFTCHILD(i)] < arr[i]) ? LEFTCHILD(i) : i ;
            if(RIGHTCHILD(i) < size && arr[RIGHTCHILD(i)] < arr[smallest]) {
                smallest = RIGHTCHILD(i);
            }
            if(smallest != i) {
                KVIntPair_t temp = arr[i];
                arr[i] = arr[smallest];
                arr[smallest] = temp;
                heapify(smallest) ;
            }
        }
        
        KVIntPair_t pop() {
            KVIntPair_t ret = arr[0];
            arr[0] = arr[--size];
            heapify(0);
            return ret;
        }
    };

    
    
    
    
    
    
    int _level;
    unsigned _pageSize; // number of elements per fence pointer
    unsigned long _runSize; // number of elts in a run
    unsigned _numRuns; // number of runs in a level
    unsigned _activeRun; // index of active run
    unsigned _mergeSize; // # of runs to merge downwards
    double _bf_fp; // bloom filter false positive
    vector<DiskRun<K,V> *> runs;

    
    
    DiskLevel<K,V>(unsigned int pageSize, int level, unsigned long runSize, unsigned numRuns, unsigned mergeSize, double bf_fp):_numRuns(numRuns), _runSize(runSize),_level(level), _pageSize(pageSize), _mergeSize(mergeSize), _activeRun(0), _bf_fp(bf_fp){
        KVPAIRMAX = (KVPair_t) {INT_MAX, 0};
        KVINTPAIRMAX = KVIntPair_t(KVPAIRMAX, -1);
        
        for (int i = 0; i < _numRuns; i++){
            DiskRun<K,V> * run = new DiskRun<K, V>(_runSize, pageSize, level, i, _bf_fp);
            runs.push_back(run);
        }

        

        
        
    }
    
    ~DiskLevel<K,V>(){
        for (int i = 0; i< runs.size(); ++i){
            delete runs[i];
        }
    }
    
    void addRuns(vector<DiskRun<K, V> *> &runList, const unsigned long runLen, bool lastLevel) {
        

        StaticHeap h = StaticHeap((int) runList.size(), KVINTPAIRMAX);
        vector<int> heads(runList.size(), 0);
        for (int i = 0; i < runList.size(); i++){
            KVPair_t kvp = runList[i]->map[0];
            h.push(KVIntPair_t(kvp, i));
        }
        
        int j = -1;
        K lastKey = INT_MAX;
        unsigned lastk = INT_MIN;
        while (h.size != 0){
            auto val_run_pair = h.pop();
            assert(val_run_pair != KVINTPAIRMAX); // TODO delete asserts
            if (lastKey == val_run_pair.first.key){
                if( lastk < val_run_pair.second){
                    runs[_activeRun]->map[j] = val_run_pair.first;
                }
            }
            else {
                ++j;
                if ( j != -1 && lastLevel && runs[_activeRun]->map[j].value == V_TOMBSTONE){
                    --j;
                }
                runs[_activeRun]->map[j] = val_run_pair.first;
            }
            
            lastKey = val_run_pair.first.key;
            lastk = val_run_pair.second;
            
            unsigned k = val_run_pair.second;
            if (++heads[k] < runList[k]->getCapacity()){
                KVPair_t kvp = runList[k]->map[heads[k]];
                h.push(KVIntPair_t(kvp, k));
            }
                
        }
        
        if (lastLevel && runs[_activeRun]->map[j].value == V_TOMBSTONE){
            --j;
        }
        runs[_activeRun]->setCapacity(j + 1);
        runs[_activeRun]->constructIndex();
        if(j + 1 > 0){
            ++_activeRun;
        }
        
    }
    
    void addRunByArray(KVPair_t * runToAdd, const unsigned long runLen){
        assert(_activeRun < _numRuns);
        assert(runLen == _runSize);
        runs[_activeRun]->writeData(runToAdd, 0, runLen);
        runs[_activeRun]->constructIndex();
        _activeRun++;
    }
    
    
    vector<DiskRun<K,V> *> getRunsToMerge(){
        vector<DiskRun<K, V> *> toMerge;
        for (int i = 0; i < _mergeSize; i++){
            toMerge.push_back(runs[i]);
        }

        return toMerge;
        
    }
    
    void freeMergedRuns(vector<DiskRun<K,V> *> &toFree){
        assert(toFree.size() == _mergeSize);
        for (int i = 0; i < _mergeSize; i++){
            assert(toFree[i]->_level == _level);
            delete toFree[i];
        }
        runs.erase(runs.begin(), runs.begin() + _mergeSize);
        _activeRun -= _mergeSize;
        for (int i = 0; i < _activeRun; i++){

            runs[i]->_runID = i;

            string newName = ("C_" + to_string(runs[i]->_level) + "_" + to_string(runs[i]->_runID) + ".txt");
            
            if (rename(runs[i]->_filename.c_str(), newName.c_str())){
                perror(("Error renaming file " + runs[i]->_filename + " to " + newName).c_str());
                exit(EXIT_FAILURE);
            }
            runs[i]->_filename = newName;
        }
        
        for (int i = _activeRun; i < _numRuns; i++){
            DiskRun<K, V> * newRun = new DiskRun<K,V>(_runSize, _pageSize, _level, i, _bf_fp);
            runs.push_back(newRun);
        }
    }
    
    bool levelFull(){
        return (_activeRun == _numRuns);
    }
    bool levelEmpty(){
        return (_activeRun == 0);
    }
    
    V lookup (const K &key, bool &found) {
        int maxRunToSearch = levelFull() ? _numRuns - 1 : _activeRun - 1;
        for (int i = maxRunToSearch; i >= 0; --i){
            if (runs[i]->maxKey == INT_MIN || key < runs[i]->minKey || key > runs[i]->maxKey || !runs[i]->bf.mayContain(&key, sizeof(K))){
                continue;
            }
            V lookupRes = runs[i]->lookup(key, found);
            if (found) {
                return lookupRes;
            }
            
        }
        
        return (V) NULL;
        
    }
    unsigned long num_elements(){
        unsigned long total = 0;
        for (int i = 0; i < _activeRun; ++i)
            total += runs[i]->getCapacity();
        return total;
    }
};
#endif /* diskLevel_h */
