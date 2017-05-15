
//    lsm.hpp
//    lsm-tree
//
//    Created by Aron Szanto on 3/3/17.


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

#ifndef LSM_H
#define LSM_H

#include "run.hpp"
#include "skipList.hpp"
#include "bloom.hpp"
#include "diskLevel.hpp"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <stdlib.h>
#include <future>
#include <vector>
#include <mutex>
#include <thread>

template <class K, class V>
class LSM {
    
    typedef SkipList<K,V> RunType;
    
    
    
public:
    V V_TOMBSTONE = (V) TOMBSTONE;
    mutex *mergeLock;
    
    vector<Run<K,V> *> C_0;
    
    vector<BloomFilter<K> *> filters;
    vector<DiskLevel<K,V> *> diskLevels;
    
    LSM<K,V>(const LSM<K,V> &other) = default;
    LSM<K,V>(LSM<K,V> &&other) = default;
    
    LSM<K,V>(unsigned long eltsPerRun, unsigned int numRuns, double merged_frac, double bf_fp, unsigned int pageSize, unsigned int diskRunsPerLevel): _eltsPerRun(eltsPerRun), _num_runs(numRuns), _frac_runs_merged(merged_frac), _diskRunsPerLevel(diskRunsPerLevel), _num_to_merge(ceil(_frac_runs_merged * _num_runs)), _pageSize(pageSize){
        _activeRun = 0;
        _bfFalsePositiveRate = bf_fp;
        _n = 0;
        
        
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
        mergeLock = new mutex();
    }
    ~LSM<K,V>(){
        if (mergeThread.joinable()){
            mergeThread.join();
        }
        delete mergeLock;
        for (int i = 0; i < C_0.size(); ++i){
            delete C_0[i];
            delete filters[i];
        }
        for (int i = 0; i < diskLevels.size(); ++i){
            delete diskLevels[i];
        }
        
    }
    
    void insert_key(K &key, V &value) {
        if (C_0[_activeRun]->num_elements() >= _eltsPerRun){
            ++_activeRun;
        }
        
        if (_activeRun >= _num_runs){
            do_merge();
        }
        
        C_0[_activeRun]->insert_key(key,value);
        filters[_activeRun]->add(&key, sizeof(K));
    }
    
    bool lookup(K &key, V &value){
        bool found = false;
        for (int i = _activeRun; i >= 0; --i){
            if (key < C_0[i]->get_min() || key > C_0[i]->get_max() || !filters[i]->mayContain(&key, sizeof(K)))
                continue;
            
            value = C_0[i]->lookup(key, found);
            if (found) {
                return value != V_TOMBSTONE;
            }
        }
        if (mergeThread.joinable()){
            // make sure that there isn't a merge happening as you search the disk
            mergeThread.join();
        }
        // it's not in C_0 so let's look at disk.
        for (int i = 0; i < _numDiskLevels; i++){
            
            value = diskLevels[i]->lookup(key, found);
            if (found) {
                return value != V_TOMBSTONE;
            }
        }
        return false;
    }
    
    void delete_key(K &key){
        insert_key(key, V_TOMBSTONE);
    }
    
    vector<KVPair<K,V>> range(K &key1, K &key2){
        if (key2 <= key1){
            return (vector<KVPair<K,V>> {});
        }
        auto ht = HashTable<K, V>(4096 * 1000);
        
        vector<KVPair<K,V>> eltsInRange = vector<KVPair<K,V>>();

        
        for (int i = _activeRun; i >= 0; --i){
            vector<KVPair<K,V>> cur_elts = C_0[i]->get_all_in_range(key1, key2);
            if (cur_elts.size() != 0){
                eltsInRange.reserve(eltsInRange.size() + cur_elts.size()); //this over-reserves to be safe
                for (int c = 0; c < cur_elts.size(); c++){
                    V dummy = ht.putIfEmpty(cur_elts[c].key, cur_elts[c].value);
                    if (!dummy && cur_elts[c].value != V_TOMBSTONE){
                        eltsInRange.push_back(cur_elts[c]);
                    }
                    
                }
            }
            
        }
        
        if (mergeThread.joinable()){
            // make sure that there isn't a merge happening as you search the disk
            mergeThread.join();
        }
        
        for (int j = 0; j < _numDiskLevels; j++){
            for (int r = diskLevels[j]->_activeRun - 1; r >= 0 ; --r){
                unsigned long i1, i2;
                diskLevels[j]->runs[r]->range(key1, key2, i1, i2);
                if (i2 - i1 != 0){
                    auto oldSize = eltsInRange.size();
                    eltsInRange.reserve(oldSize + (i2 - i1)); // also over-reserves space
                    for (unsigned long m = i1; m < i2; ++m){
                        auto KV = diskLevels[j]->runs[r]->map[m];
                        V dummy = ht.putIfEmpty(KV.key, KV.value);
                        if (!dummy && KV.value != V_TOMBSTONE) {
                            eltsInRange.push_back(KV);
                        }
                    }
                }
            }
        }
        
        return eltsInRange;
    }
    
    
    
    void printElts(){
        if (mergeThread.joinable())
            mergeThread.join();
        cout << "MEMORY BUFFER" << endl;
        for (int i = 0; i <= _activeRun; i++){
            cout << "MEMORY BUFFER RUN " << i << endl;
            auto all = C_0[i]->get_all();
            for (KVPair<K, V> &c : all) {
                cout << c.key << ":" << c.value << " ";
            }
            cout << endl;
            
        }
        
        cout << "\nDISK BUFFER" << endl;
        for (int i = 0; i < _numDiskLevels; i++){
            cout << "DISK LEVEL " << i << endl;
            for (int j = 0; j < diskLevels[i]->_activeRun; j++){
                cout << "RUN " << j << endl;
                for (int k = 0; k < diskLevels[i]->runs[j]->getCapacity(); k++){
                    cout << diskLevels[i]->runs[j]->map[k].key << ":" << diskLevels[i]->runs[j]->map[k].value << " ";
                }
                cout << endl;
            }
            cout << endl;
        }
        
    }
    void printStats(){
        cout << "Number of Elements: " << size() << endl;
        cout << "Number of Elements in Buffer (including deletes): " << num_buffer() << endl;
        
        for (int i = 0; i < diskLevels.size(); ++i){
            cout << "Number of Elements in Disk Level " << i << "(including deletes): " << diskLevels[i]->num_elements() << endl;
        }
        cout << "KEY VALUE DUMP BY LEVEL: " << endl;
        printElts();
    }
    
    //private: // TODO MAKE PRIVATE
    unsigned int _activeRun;
    unsigned long _eltsPerRun;
    double _bfFalsePositiveRate;
    unsigned int _num_runs;
    double _frac_runs_merged;
    unsigned int _numDiskLevels;
    unsigned int _diskRunsPerLevel;
    unsigned int _num_to_merge;
    unsigned int _pageSize;
    unsigned long _n;
    thread mergeThread;
    
    void mergeRunsToLevel(int level) {
        bool isLast = false;
        
        if (level == _numDiskLevels){ // if this is the last level
            DiskLevel<K,V> * newLevel = new DiskLevel<K, V>(_pageSize, level + 1, diskLevels[level - 1]->_runSize * diskLevels[level - 1]->_mergeSize, _diskRunsPerLevel, ceil(_diskRunsPerLevel * _frac_runs_merged), _bfFalsePositiveRate);
            diskLevels.push_back(newLevel);
            _numDiskLevels++;
        }
        
        if (diskLevels[level]->levelFull()) {
            mergeRunsToLevel(level + 1); // merge down one, recursively
        }
        
        if(level + 1 == _numDiskLevels && diskLevels[level]->levelEmpty()){
            isLast = true;
        }
        
        
        vector<DiskRun<K, V> *> runsToMerge = diskLevels[level - 1]->getRunsToMerge();
        unsigned long runLen = diskLevels[level - 1]->_runSize;
        diskLevels[level]->addRuns(runsToMerge, runLen, isLast);
        diskLevels[level - 1]->freeMergedRuns(runsToMerge);
        

        
        
        
    }
    void merge_runs(vector<Run<K,V>*> runs_to_merge, vector<BloomFilter<K>*> bf_to_merge){
        vector<KVPair<K, V>> to_merge = vector<KVPair<K,V>>();
        to_merge.reserve(_eltsPerRun * _num_to_merge);
        for (int i = 0; i < runs_to_merge.size(); i++){
            auto all = (runs_to_merge)[i]->get_all();
            
            to_merge.insert(to_merge.begin(), all.begin(), all.end());
            delete (runs_to_merge)[i];
            delete (bf_to_merge)[i];
        }
        sort(to_merge.begin(), to_merge.end());
        mergeLock->lock();
        if (diskLevels[0]->levelFull()){
            mergeRunsToLevel(1);
        }
        diskLevels[0]->addRunByArray(&to_merge[0], to_merge.size());
        mergeLock->unlock();
        
    }
    
    void do_merge(){
        if (_num_to_merge == 0)
            return;
        vector<Run<K,V>*> runs_to_merge = vector<Run<K,V>*>();
        vector<BloomFilter<K>*> bf_to_merge = vector<BloomFilter<K>*>();
        for (int i = 0; i < _num_to_merge; i++){
            runs_to_merge.push_back(C_0[i]);
            bf_to_merge.push_back(filters[i]);
        }
        if (mergeThread.joinable()){
            mergeThread.join();
        }
        mergeThread = thread (&LSM::merge_runs, this, runs_to_merge,bf_to_merge); // comment for single threaded merging
//        merge_runs(runs_to_merge, bf_to_merge); // uncomment for single threaded merging
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
    }
    unsigned long num_buffer(){
        if (mergeThread.joinable())
            mergeThread.join();
        unsigned long total = 0;
        for (int i = 0; i <= _activeRun; ++i)
            total += C_0[i]->num_elements();
        return total;
    }
    unsigned long size(){
        K min = INT_MIN;
        K max = INT_MAX;
        auto r = range(min, max);
        return r.size();
    }

};




#endif /* lsm_h */

