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

#define PAGESIZE 4096

using namespace std;

template <class K, class V>
class DiskLevel {
public:
    
    
    DiskLevel<K,V> (unsigned long long capacity, unsigned long long numElts, int level, vector<KVPair<K,V>> &pairs):_capacity(capacity),_numElts(numElts),_level(level) {
        _filename = ("C_" + to_string(level) + ".txt").c_str();
        size_t filesize = capacity * sizeof(KVPair<K,V>);
        int i;
        int fd;
        long result;
        KVPair<K, V> *map;  /* mmapped array of KVPairs */
        
        /* Open a file for writing.
         *  - Creating the file if it doesn't exist.
         *  - Truncating it to 0 size if it already exists. (not really needed)
         *
         * Note: "O_WRONLY" mode is not sufficient when mmaping.
         */
        fd = open(_filename, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
        if (fd == -1) {
            perror("Error opening file for writing");
            exit(EXIT_FAILURE);
        }
        
        /* Stretch the file size to the size of the (mmapped) array of ints
         */
        result = lseek(fd, filesize - 1, SEEK_SET);
        if (result == -1) {
            close(fd);
            perror("Error calling lseek() to 'stretch' the file");
            exit(EXIT_FAILURE);
        }
        
        /* Something needs to be written at the end of the file to
         * have the file actually have the new size.
         * Just writing an empty string at the current file position will do.
         *
         * Note:
         *  - The current position in the file is at the end of the stretched
         *    file due to the call to lseek().
         *  - An empty string is actually a single '\0' character, so a zero-byte
         *    will be written at the last byte of the file.
         */
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
        
        /* Now write int's to the file as if it were memory (an array of ints).
         */
        for (i = 0; i < numElts; ++i) {
            map[i] = pairs[i];
        }
        
        /* Don't forget to free the mmapped memory
         */
        if (munmap(map, filesize) == -1) {
            perror("Error un-mmapping the file");
        }
        
        /* Un-mmaping doesn't close the file, so we still need to do that.
         */
        close(fd);
        
        
        
        
        
        
        fd = open(_filename, O_RDONLY);
        if (fd == -1) {
            perror("Error opening file for reading");
            exit(EXIT_FAILURE);
        }
        
        map = (KVPair<K,V>*) mmap(0, numElts * sizeof(KVPair<K,V>), PROT_READ, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) {
            close(fd);
            perror("Error mmapping the file");
            exit(EXIT_FAILURE);
        }
        
        /* Read the file int-by-int from the mmap
         */
        for (i = 0; i < numElts; ++i) {
            printf("I%d: K %d V %d \n", i, map[i].key, map[i].value);
        }
        
            if (munmap(map, numElts * sizeof(KVPair<K,V>)) == -1) {
            perror("Error un-mmapping the file");
        }
        close(fd);
    
    }
    void updateFencePointers(){
        return;
    }
private:
    unsigned long long _capacity;
    unsigned long long _numElts;
    const char  *_filename;
    int _level;
    vector<K> _fencePointers;
    
};
#endif /* diskLevel_h */
