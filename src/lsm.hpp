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



template <class K, class V>
class LSM {
    
    typedef SkipList<K,V> RunType;
    
    
    
public:
    vector<Run<K,V> *> C_0;
    
    vector<BloomFilter<K> *> filters;
    vector<DiskLevel<K,V> *> diskLevels;
    
    LSM<K,V>(unsigned long initialSize, unsigned int numRuns, double sizeRatio, double merged_frac, double bf_fp, unsigned int pageSize, unsigned int diskRunsPerLevel):_sizeRatio(sizeRatio),_initialSize(initialSize), _num_runs(numRuns), _frac_runs_merged(merged_frac), _diskRunsPerLevel(diskRunsPerLevel), _num_to_merge(ceil(_frac_runs_merged * _num_runs)){
        _activeRun = 0;
        _eltsPerRun = initialSize / numRuns;
        _bfFalsePositiveRate = bf_fp;
        
        
        DiskLevel<K,V> * diskLevel = new DiskLevel<K, V>(pageSize, 1, _num_to_merge * _eltsPerRun, _diskRunsPerLevel, ceil(_diskRunsPerLevel * _frac_runs_merged));

        diskLevels.push_back(diskLevel);
        _activeDiskLevel = 0;
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
        // TODO keep track of min/max in runs?
//        cout << "looking for key " << key << endl;
        for (int i = _activeRun; i >= 0; --i){
//            cout << "... in run/filter " << i << endl;
            if (!filters[i]->mayContain(&key, sizeof(K)))
                continue;
            
            V lookupRes = C_0[i]->lookup(key);
            if (lookupRes)
                return lookupRes;
        }
        // it's not in C_0 so let's look at disk.
        for (int i = _activeDiskLevel; i >= 0; --i){
            
            V lookupRes = diskLevels[i]->lookup(key);
            if (lookupRes)
                return lookupRes;
        }

        return NULL;
    }
    
    
    unsigned long long num_elements(){
        unsigned long long total = 0;
        for (int i = 0; i <= _activeRun; ++i)
            total += C_0[i]->num_elements();
        return total;
    }
    
//private: // TODO MAKE PRIVATE
    double _sizeRatio;
    unsigned long _initialSize;
    unsigned int _activeRun;
    unsigned long _eltsPerRun;
    double _bfFalsePositiveRate;
    unsigned int _num_runs;
    double _frac_runs_merged;
    unsigned int _numDiskLevels;
    unsigned int _activeDiskLevel;
    unsigned int _diskRunsPerLevel;
    unsigned int _num_to_merge;
    
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
        diskLevels[0]->merge(&to_merge[0], to_merge.size());
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
    
};




#endif /* lsm_h */

