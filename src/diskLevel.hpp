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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cassert>

#define PAGESIZE 4096

using namespace std;

template <class K, class V>
class DiskLevel {
public:
    typedef KVPair<K,V> KVPair_t;
    
    DiskLevel<K,V> (unsigned long long capacity, unsigned long long numElts, int level, KVPair_t *pairs):_capacity(capacity),_numElts(numElts),_level(level), _iMaxFP(0) {
        
        _filename = ("C_" + to_string(level) + ".txt").c_str();
        
        size_t filesize = capacity * sizeof(KVPair_t);

        int i;
        int fd;
        long result;
        KVPair<K, V> *map;  /* mmapped array of KVPairs */
        
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
        
        /* Now the file is ready to be mmapped.
         */
        map = (KVPair<K, V>*) mmap(0, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) {
            close(fd);
            perror("Error mmapping the file");
            exit(EXIT_FAILURE);
        }
        
        /* Now write KVPairs to the file as if it were memory (an array of KVPairs).
         */
        // TODO: Is this safe?
        memcpy(map, &pairs[0], numElts * sizeof(KVPair_t));
        
        /*free the mmapped memory
         */
        if (munmap(map, filesize) == -1) {
            perror("Error un-mmapping the file");
        }
        
        close(fd);
        
        _fencePointers.resize(0);
        for (int j = 0; j * PAGESIZE < _numElts; j++) {
            _fencePointers.push_back(pairs[j * PAGESIZE].key);
            _iMaxFP++;
        }

        
        
        
    
    }
    void updateFencePointers(){
        _fencePointers.resize(_numElts / PAGESIZE);
        
        size_t filesize = _numElts * sizeof(KVPair_t);
        
        int i;
        int fd;
        long result;
        KVPair<K, V> *map;  /* mmapped array of KVPairs */

        
        fd = open(_filename, O_RDONLY);
        if (fd == -1) {
            perror("Error opening file for reading");
            exit(EXIT_FAILURE);
        }
        
        map = (KVPair_t*) mmap(0, _numElts * sizeof(KVPair_t), PROT_READ, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) {
            close(fd);
            perror("Error mmapping the file");
            exit(EXIT_FAILURE);
        }
        
        /* Read the file int-by-int from the mmap
         */
        for (i = 0; i < _numElts; i++) {
            _fencePointers[i] = map[i * PAGESIZE].key;
        }
        
        if (munmap(map, _numElts * sizeof(KVPair_t)) == -1) {
            perror("Error un-mmapping the file");
        }
        close(fd);

        
    }
private:
    unsigned long long _capacity;
    unsigned long long _numElts;
    const char  *_filename;
    int _level;
    vector<K> _fencePointers;
    unsigned _iMaxFP;
    
    
    
    
};
#endif /* diskLevel_h */
