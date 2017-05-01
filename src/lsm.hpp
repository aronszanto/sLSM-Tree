//
//  lsm.hpp
//  lsm-tree
//
//  Created by Aron Szanto on 3/3/17.
//  Copyright © 2017 Aron Szanto. All rights reserved.
//

#ifndef LSM_H
#define LSM_H

#include "run.hpp"
#include "skipList.hpp"
#include "bloom.hpp"
#include "diskLevel.hpp"
#include <cstdio>
#include <cstdint>
#include <vector>

const int TOMBSTONE = INT_MIN;

template <class K, class V>
class LSM {
    
    typedef SkipList<K,V> RunType;
    
    
    
public:
    V V_TOMBSTONE = (V) TOMBSTONE;
    vector<Run<K,V> *> C_0;
    
    vector<BloomFilter<K> *> filters;
    vector<DiskLevel<K,V> *> diskLevels;
    
    LSM<K,V>(unsigned long initialSize, unsigned int numRuns, double merged_frac, double bf_fp, unsigned int pageSize, unsigned int diskRunsPerLevel):_initialSize(initialSize), _num_runs(numRuns), _frac_runs_merged(merged_frac), _diskRunsPerLevel(diskRunsPerLevel), _num_to_merge(ceil(_frac_runs_merged * _num_runs)), _pageSize(pageSize){
        _activeRun = 0;
        _eltsPerRun = initialSize / numRuns;
        _bfFalsePositiveRate = bf_fp;
        
        
        DiskLevel<K,V> * diskLevel = new DiskLevel<K, V>(pageSize, 1, _num_to_merge * _eltsPerRun, _diskRunsPerLevel, ceil(_diskRunsPerLevel * _frac_runs_merged), _bfFalsePositiveRate);

        diskLevels.push_back(diskLevel);
        _numDiskLevels = 1;
        
        
        for (int i = 0; i < _num_runs; i++){
            RunType * run = new RunType(INT32_MIN,INT32_MAX);
            run->set_size(_eltsPerRun);
            C_0.push_back(run);
            
            BloomFilter<K> * bf = new BloomFilter<K>(_eltsPerRun, _bfFalsePositiveRate);
            filters.push_back(bf);
        }
    }
    
    void insert_key(K key, V value) {
//        cout << "inserting key " << key << endl;
        if (C_0[_activeRun]->num_elements() >= _eltsPerRun){
//            cout << "run " << _activeRun << " full, moving to next" << endl;
            ++_activeRun;
        }
        
        if (_activeRun >= _num_runs){
//            cout << "need to merge" << endl;
            do_merge();
        }
        
//        cout << "inserting key " << key << " to run " << _activeRun << endl;
        C_0[_activeRun]->insert_key(key,value);
        filters[_activeRun]->add(&key, sizeof(K));
    }
    
    V lookup(K key){
        bool found = false;
        // TODO keep track of min/max in runs?
//        cout << "looking for key " << key << endl;
        for (int i = _activeRun; i >= 0; --i){
//            cout << "... in run/filter " << i << endl;
            if (key < C_0[i]->get_min() || key > C_0[i]->get_max() || !filters[i]->mayContain(&key, sizeof(K)))
                continue;
            
            V lookupRes = C_0[i]->lookup(key, &found);
            if (found) {
                return lookupRes == V_TOMBSTONE ? NULL : lookupRes;
            }
        }
        // it's not in C_0 so let's look at disk.
        for (int i = 0; i < _numDiskLevels; i++){
            
            V lookupRes = diskLevels[i]->lookup(key, &found);
            if (found) {
                return lookupRes == V_TOMBSTONE ? NULL : lookupRes;
            }
        }

        return NULL;
    }
    
    void delete_key(K key){
        insert_key(key, V_TOMBSTONE);
    }
    
    vector<KVPair<K,V>> range(K key1, K key2){
        if (key2 <= key1){
            return (vector<KVPair<K,V>> {});
        }
        auto ht = HashTable<K, V>(1024);
        vector<KVPair<K,V>> eltsInRange = vector<KVPair<K,V>>();
        for (int i = _activeRun; i >= 0; --i){
            vector<KVPair<K,V>> cur_elts = C_0[i]->get_all_in_range(key1, key2);
            if (cur_elts.size() != 0){
                eltsInRange.reserve(eltsInRange.size() + cur_elts.size()); //this over-reserves to be safe
                for (int c = 0; c < cur_elts.size(); c++){
                    V dummy;
                    if (!ht.get(cur_elts[c].key, dummy)){
                        ht.put(cur_elts[c].key, cur_elts[c].value);
                        if (dummy != V_TOMBSTONE && cur_elts[c].value != V_TOMBSTONE){
                            eltsInRange.push_back(cur_elts[c]);
                        }
                    }
                    
                }
            }
            
        }
        
        for (int j = 0; j < _numDiskLevels; j++){
            for (int r = diskLevels[j]->_activeRun - 1; r >= 0 ; --r){
                unsigned long i1, i2;
                diskLevels[j]->runs[r]->range(key1, key2, i1, i2);
                if (i2 - i1 != 0){
                    auto oldSize = eltsInRange.size();
                    eltsInRange.reserve(oldSize + (i2 - i1)); // also over-reserve space
                    for (unsigned long m = i1; m < i2; ++m){
                        auto KV = diskLevels[j]->runs[r]->map[m];
                        V dummy;
                        if (!ht.get(KV.key, dummy)){
                            ht.put(KV.key, KV.value);
                            if (dummy != V_TOMBSTONE && KV.value != V_TOMBSTONE){
                                eltsInRange.push_back(KV);
                            }
                            
                        }
                    }                    
                }
                
            }
        }
        
        return eltsInRange;
    }
    
    
    unsigned long long num_elements(){
        unsigned long long total = 0;
        for (int i = 0; i <= _activeRun; ++i)
            total += C_0[i]->num_elements(); // TODO NEED TO GET DISK ELTS TOO
        return total;
    }
    
    void printElts(){
        cout << "MEMORY BUFFER" << endl;
        for (int i = 0; i <= _activeRun; i++){
            cout << "MEMORY BUFFER RUN " << i << endl;
            auto all = C_0[i]->get_all();
            for (KVPair<K, V> &c : all) {
                cout << c.key << " ";
            }
            cout << endl;

        }
        
        cout << "\nDISK BUFFER" << endl;
        for (int i = 0; i < _numDiskLevels; i++){
            cout << "DISK LEVEL " << i << endl;
            for (int j = 0; j < _diskRunsPerLevel; j++){
                cout << "RUN " << j << endl;
                for (int k = 0; k < diskLevels[i]->_runSize; k++){
                    cout << diskLevels[i]->runs[j]->map[k].key << " ";
                }
                cout << endl;
            }
            cout << endl;
        }
    }
    
//private: // TODO MAKE PRIVATE
    unsigned long _initialSize;
    unsigned int _activeRun;
    unsigned long _eltsPerRun;
    double _bfFalsePositiveRate;
    unsigned int _num_runs;
    double _frac_runs_merged;
    unsigned int _numDiskLevels;
    unsigned int _diskRunsPerLevel;
    unsigned int _num_to_merge;
    unsigned int _pageSize;
    
    void mergeRunsToLevel(int level) {
//        cout << "loc 3" << endl;
//        diskLevels[0]->runs[0]->printElts();
        if (level == _numDiskLevels){ // if this is the last level
//            cout << "loc 4" << endl;
//            diskLevels[0]->runs[0]->printElts();
//            cout << "adding a new level: " << level << endl;
//            cout << "new level runsize: " <<  diskLevels[level - 1]->_runSize * diskLevels[level - 1]->_mergeSize << endl;
            DiskLevel<K,V> * newLevel = new DiskLevel<K, V>(_pageSize, level + 1, diskLevels[level - 1]->_runSize * diskLevels[level - 1]->_mergeSize, _diskRunsPerLevel, ceil(_diskRunsPerLevel * _frac_runs_merged), _bfFalsePositiveRate);
//            cout << "loc 5" << endl;
//            diskLevels[0]->runs[0]->printElts();
            diskLevels.push_back(newLevel);
            _numDiskLevels++;
//            cout << "loc 6" << endl;
//            diskLevels[0]->runs[0]->printElts();

        }
        
        if (diskLevels[level]->levelFull()) {
//            cout << "level " << level << " full, cascading" << endl;
            mergeRunsToLevel(level + 1); // merge down one, recursively
        }
        

//        cout << "writing values from level " << (level - 1) << " to level " << level << endl;
        vector<DiskRun<K, V> *> runsToMerge = diskLevels[level - 1]->getRunsToMerge();
        unsigned long runLen = diskLevels[level - 1]->_runSize;
//        cout << "values to write from level " << level - 1 << ": " << endl;
//        for (int i = 0; i < runsToMerge.size(); i++){
//            for (int j = 0; j < diskLevels[level - 1]->_runSize; j++){
//                cout << runsToMerge[i]->map[j].key << " ";
//            }
//            cout << endl;
//        }
        diskLevels[level]->addRuns(runsToMerge, runLen);
        diskLevels[level - 1]->freeMergedRuns(runsToMerge);


        
    }
    
    void do_merge(){
        
        if (_num_to_merge == 0)
            return;
//        cout << "going to merge " << num_to_merge << " runs" << endl;
        vector<KVPair<K, V>> to_merge = vector<KVPair<K,V>>();
        to_merge.reserve(_eltsPerRun * _num_to_merge);
        for (int i = 0; i < _num_to_merge; i++){
//            cout << "grabbing values in and deleting run " << i << endl;
            auto all = C_0[i]->get_all();
//            cout << "values in run " << i << endl;
            
            to_merge.insert(to_merge.begin(), all.begin(), all.end());
            delete C_0[i];
            delete filters[i];
        }
        sort(to_merge.begin(), to_merge.end());
//        cout << "merging to disk" << endl;
        
        if (diskLevels[0]->levelFull()){
            mergeRunsToLevel(1);
        }
        
        
        diskLevels[0]->addRunByArray(&to_merge[0], to_merge.size());
    
        C_0.erase(C_0.begin(), C_0.begin() + _num_to_merge);
        filters.erase(filters.begin(), filters.begin() + _num_to_merge);

        _activeRun -= _num_to_merge;
        for (int i = _activeRun; i < _num_runs; i++){
            RunType * run = new RunType(INT32_MIN,INT32_MAX);
            run->set_size(_eltsPerRun);
            C_0.push_back(run);
            
            BloomFilter<K> * bf = new BloomFilter<K>(_eltsPerRun, _bfFalsePositiveRate);
            filters.push_back(bf);
        }
//        cout << "finished merging- report: " << endl;
//        printElts();
        

    }
    
};




#endif /* lsm_h */

