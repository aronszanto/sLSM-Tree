# sLSM-Tree

Research Abstract:
Log-Structured Merge (LSM) Trees provide a tiered data storage and retrieval paradigm that is attractive for write-optimized data systems. Maintaining an efficient buffer in memory and deferring updates past their initial write-time, the structure provides quick operations over hot data. Because each layer of the structure is logically separate from the others, the structure is also conducive to opportunistic and granular optimization. In this project, I introduce the Skiplist-Based LSM Tree (sLSM), a novel system in which the memory buffer of the LSM is composed of a sequence of skiplists. I develop theoretical and experimental results that demonstrate that the breadth of tuning parameters inherent to the sLSM allows it broad flexibility for excellent performance across a wide variety of workloads.

## Notes
This project was written entirely in native C and C++, with no dependencies. This means that: 1. there is no part of the code that is left to guesswork; 2. the project is modular enough to swap out components (hash tables, skiplists, etc.) for other ones if desired; and 3. the sLSM Tree is effectively limitlessly portable.

## Performance
On commodity hardware, test performance was up to 1 million writes per second and 6 million reads per second. Details in the paper!
