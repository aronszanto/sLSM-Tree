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
    DiskLevel<K,V> disk_level;
    
    LSM<K,V>(unsigned long initialSize, unsigned int numRuns, double sizeRatio, double merged_frac, double bf_fp, unsigned int pageSize):_sizeRatio(sizeRatio),_initialSize(initialSize), _num_runs(numRuns), disk_level(initialSize * sizeRatio, pageSize, 1), _frac_runs_merged(merged_frac){
        _activeRun = 0;
        _eltsPerRun = initialSize / numRuns;
        _bfFalsePositiveRate = bf_fp;
        
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
        
        V looked = disk_level.lookup(key);
        if (looked){
            return looked;
        }
        else{
            return NULL;
        }
    }
    
    
    unsigned long long num_elements(){
        unsigned long long total = 0;
        for (int i = 0; i <= _activeRun; ++i)
            total += C_0[i]->num_elements();
        return total;
    }
    
    // how do you do disk stuff?
//private: // TODO MAKE PRIVATE
    double _sizeRatio;
    unsigned long _initialSize;
    unsigned int _activeRun;
    unsigned long _eltsPerRun;
    double _bfFalsePositiveRate;
    unsigned int _num_runs;
    double _frac_runs_merged;
    
    void do_merge(){
        int num_to_merge = _frac_runs_merged * _num_runs;
        if (num_to_merge == 0)
            return;
//        cout << "going to merge " << num_to_merge << " runs" << endl;
        vector<KVPair<K, V>> to_merge = vector<KVPair<K,V>>();
        to_merge.reserve(_eltsPerRun * num_to_merge);
        for (int i = 0; i < num_to_merge; i++){
//            cout << "grabbing values in and deleting run " << i << endl;
            auto all = C_0[i]->get_all();
//            cout << "values in run " << i << endl;
            
            to_merge.insert(to_merge.begin(), all.begin(), all.end());
            delete C_0[i];
        }
        sort(to_merge.begin(), to_merge.end());
//        cout << "merging to disk" << endl;
        disk_level.merge(&to_merge[0], to_merge.size());
        C_0.erase(C_0.begin(), C_0.begin() + num_to_merge);
        filters.erase(filters.begin(), filters.begin() + num_to_merge);

        _activeRun -= num_to_merge;
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

