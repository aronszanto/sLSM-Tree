//
//  diskLevel.hpp
//  lsm-tree
//
//  Created by Aron Szanto on 3/20/17.
//  Copyright © 2017 Aron Szanto. All rights reserved.
//

#ifndef diskLevel_h
#define diskLevel_h
#include <vector>
#include <cstdint>
#include <string>
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
#include "imsort.hpp"


using namespace std;

template <class K, class V>
class DiskLevel {
public: // TODO make some of these private
    int _level;
    unsigned _pageSize; // number of elements per fence pointer
    unsigned long _runSize; // number of elts in a run
    unsigned _numRuns; // number of runs in a level
    unsigned _activeRun; // index of active run
    unsigned _mergeSize; // # of runs to merge downwards
    vector<DiskRun<K,V> *> runs;

    typedef KVPair<K,V> KVPair_t;
    
    
    DiskLevel<K,V>(unsigned int pageSize, int level, unsigned long runSize, unsigned numRuns, unsigned mergeSize):_numRuns(numRuns), _runSize(runSize),_level(level), _pageSize(pageSize), _mergeSize(mergeSize), _activeRun(0){
        
        
        
        for (int i = 0; i < _numRuns; i++){
            DiskRun<K,V> * run = new DiskRun<K, V>(_runSize, pageSize, level, i);
            runs.push_back(run);
        }

        

        
        
    }
    
    ~DiskLevel<K,V>(){
        }
    
    void addRuns(vector<KVPair_t *> &runList, const unsigned long len) {
        assert(_activeRun < _numRuns);
        assert(len * runList.size() == runs[_activeRun]->runSize);
        
        for (int i = 0; i < runList.size(); i++){
            runs[_activeRun]->writeData(runList[i], i * len, len);
        }
        _activeRun++;
        
    }
    
    void addRun(KVPair_t * runToAdd, const unsigned long len){
        assert(_activeRun < _numRuns);
        assert(len == runs[_activeRun]->runSize);
        runs[_activeRun]->writeData(runToAdd, 0, len);
        _activeRun++;
    }
    
    
    vector<KVPair_t *> getRunsToMerge(){
        vector<KVPair_t *> toMerge;
        for (int i = 0; i < _mergeSize; i++){
            toMerge.push_back(runs[i]);
        }
        return toMerge;
        
    }
    
    void freeMergedRuns(vector<KVPair_t *> &toFree){
        assert(toFree.size() == _mergeSize);
        for (int i = 0; i < _mergeSize; i++){
            delete toFree[i];
        }
        runs.erase(runs.begin(), runs.begin() + _mergeSize);
        _activeRun -= _mergeSize;
    }
};
#endif /* diskLevel_h */
