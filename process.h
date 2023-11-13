#ifndef PROCESS_H
#define PROCESS_H

#include "TwoLevelPageTable.h"
#include <cstdint>

class process {
public:
    long int pid;
    long int size;
    long int heapPages;
    uint32_t code;
    uint32_t stack;
    uint32_t heap;
    TwoLevelPageTable pageTable;

    process(long int pidGiven);

    void allocateMem(uint32_t allocatedSize);
    void freeMem(uint32_t freedSize);
};

#endif