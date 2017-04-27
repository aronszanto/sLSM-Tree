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
    
    void addRuns(vector<DiskRun<K, V> *> &runList, const unsigned long runLen) {
        assert(_activeRun < _numRuns);
        assert(runLen * runList.size() == _runSize);
        
        for (int i = 0; i < runList.size(); i++){
            runs[_activeRun]->writeData(runList[i]->map, i * runLen, runLen);
        }
        runs[_activeRun]->writeFencePointers();
        _activeRun++;
        
    }
    
    void addRunByArray(KVPair_t * runToAdd, const unsigned long runLen){
        assert(_activeRun < _numRuns);
        assert(runLen == _runSize);
        runs[_activeRun]->writeData(runToAdd, 0, runLen);
        runs[_activeRun]->writeFencePointers();
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
            delete toFree[i];
        }
        runs.erase(runs.begin(), runs.begin() + _mergeSize);
        _activeRun -= _mergeSize;
        
        for (int i = _activeRun; i < _numRuns; i++){
            DiskRun<K, V> * newRun = new DiskRun<K,V>(_runSize, _pageSize, _level, i);
            runs.push_back(newRun);
        }
    }
    
    bool levelFull(){
        return (_activeRun == _numRuns);
    }
    
    V lookup (K key) {
        for (int i = _activeRun; i >= 0; --i){
//            if (!filters[i]->mayContain(&key, sizeof(K)))
//                continue;
            // TODO PUT BFs HERE!
            V lookupRes = runs[i]->lookup(key);
            if (lookupRes) {
                return lookupRes;
            }
            
        }
        
        return NULL;
        
    }
};
#endif /* diskLevel_h */
