//
//  diskRun.hpp
//  lsm-tree
//
//  Created by Aron Szanto on 4/26/17.
//  Copyright © 2017 Aron Szanto. All rights reserved.
//

//
//  diskLevel.hpp
//  lsm-tree
//
//  Created by Aron Szanto on 3/20/17.
//  Copyright © 2017 Aron Szanto. All rights reserved.
//

#ifndef diskRun_h
#define diskRun_h
#include <vector>
#include <cstdint>
#include <string>
#include "run.hpp"
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
class DiskRun {
public:
    typedef KVPair<K,V> KVPair_t;
    
    static int compareKVs (const void * a, const void * b)
    {
        if ( *(KVPair<K,V>*)a <  *(KVPair<K,V>*)b ) return -1;
        if ( *(KVPair<K,V>*)a == *(KVPair<K,V>*)b ) return 0;
        if ( *(KVPair<K,V>*)a >  *(KVPair<K,V>*)b ) return 1;
        return 10;
    }
    
    
    KVPair_t *map;
    int fd;
    unsigned int pageSize;
    
    DiskRun<K,V> (unsigned long long capacity, unsigned int pageSize, int level, int runID):_capacity(capacity),_level(level), _iMaxFP(0), pageSize(pageSize) {
        
        _filename = ("C_" + to_string(level) + "_" + to_string(runID) + ".txt").c_str();
        
        size_t filesize = capacity * sizeof(KVPair_t);
        
        long result;
        
        fd = open(_filename, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
        if (fd == -1) {
            perror("Error opening file for writing");
            exit(EXIT_FAILURE);
        }
        
        /* Stretch the file size to the size of the (mmapped) array of KVPairs
         */
        result = lseek(fd, filesize - 1, SEEK_SET);
        if (result == -1) {
            close(fd);
            perror("Error calling lseek() to 'stretch' the file");
            exit(EXIT_FAILURE);
        }
        
        
        result = write(fd, "", 1);
        if (result != 1) {
            close(fd);
            perror("Error writing last byte of the file");
            exit(EXIT_FAILURE);
        }
        
        
        map = (KVPair<K, V>*) mmap(0, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) {
            close(fd);
            perror("Error mmapping the file");
            exit(EXIT_FAILURE);
        }
        
        
        
    }
    ~DiskRun<K,V>(){
        doUnmap();
    }
    
    void writeData(const KVPair_t *run, const size_t offset, const unsigned long len) {
        
        memcpy(map + offset, run, len * sizeof(KVPair_t));        
        
    }
    void writeFencePointers(){
        // construct fence pointers
        _fencePointers.resize(0);
        _iMaxFP = -1; // TODO IS THIS SAFE?
        for (int j = 0; j * pageSize < _capacity; j++) {
            _fencePointers.push_back(map[j * pageSize].key);
            _iMaxFP++;
        }
    }
    
    KVPair_t binary_search (const int offset, int n, KVPair_t key, bool *found) {
        
        
        int min = offset, max = offset + n;
        while (min < max) {
            int middle = (min + max) >> 1;
            if (key > map[middle])
                min = middle + 1;
            else if (key < map[middle])
                max = middle;
            else {
                *found = true;
                return map[middle];
            }
            
        }
        return (KVPair_t) {0,0}; // TODO THIS IS GROSS
    }
    V lookup(K key, bool *found){
        KVPair_t k = {key, 0};
        
        unsigned long long start;
        int end;
        
        if (_iMaxFP == 0) {
            start = 0;
            end = _capacity;
        }
        else if (key <= _fencePointers[1]){
            start = 0;
            end = pageSize;
        }
        else if (key >= _fencePointers[_iMaxFP]) {
            start = _iMaxFP * pageSize;
            end = _capacity;
        }
        else {
            unsigned min = 0, max = _iMaxFP;
            while (min < max) {
                
                unsigned middle = (min + max) >> 1;
                if (key > _fencePointers[middle]){
                    if (key <= _fencePointers[middle + 1]){
                        start = middle * pageSize;
                        end = (middle + 1) * pageSize;
                        break; // TODO THIS IS ALSO GROSS
                    }
                    min = middle + 1;
                }
                else if (key < _fencePointers[middle]) {
                    if (key >= _fencePointers[middle - 1]){
                        start = (middle - 1) * pageSize;
                        end = middle * pageSize;
                        break; // TODO THIS IS ALSO GROSS. THIS WILL BREAK IF YOU DON'T KEEP TRACK OF MIN AND MAX.
                    }
                    
                    max = middle - 1;
                }
                
                else {
                    *found = true;
                    return map[middle * pageSize].value;
                }
                
            }
        }
        
        return binary_search(start, end - start, k, found).value;
    }
    
    
private:
    unsigned long long _capacity;
    const char  *_filename;
    int _level;
    vector<K> _fencePointers;
    unsigned _iMaxFP;
    
    void doMap(){
        
        size_t filesize = _capacity * sizeof(KVPair_t);
        
        fd = open(_filename, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
        if (fd == -1) {
            perror("Error opening file for writing");
            exit(EXIT_FAILURE);
        }
        
        
        map = (KVPair<K, V>*) mmap(0, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) {
            close(fd);
            perror("Error mmapping the file");
            exit(EXIT_FAILURE);
        }
    }
    
    void doUnmap(){
        size_t filesize = _capacity * sizeof(KVPair_t);
        
        if (munmap(map, filesize) == -1) {
            perror("Error un-mmapping the file");
        }
        
        close(fd);
        fd = -5;
    }
    
    void doubleSize(){
        unsigned long long new_capacity = _capacity * 2;
        
        size_t new_filesize = new_capacity * sizeof(KVPair_t);
        int result = lseek(fd, new_filesize - 1, SEEK_SET);
        if (result == -1) {
            close(fd);
            perror("Error calling lseek() to 'stretch' the file");
            exit(EXIT_FAILURE);
        }
        
        result = write(fd, "", 1);
        if (result != 1) {
            close(fd);
            perror("Error writing last byte of the file");
            exit(EXIT_FAILURE);
        }
        
        map = (KVPair<K, V>*) mmap(0, new_filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) {
            close(fd);
            perror("Error mmapping the file");
            exit(EXIT_FAILURE);
        }
        
        _capacity = new_capacity;
    }
    
    
    
    
};
#endif /* diskRun_h */

