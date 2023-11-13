// process.cpp
#include "process.h"
#include "TwoLevelPageTable.h"
#include <iostream>

using namespace std;

process::process(long int pidGiven) : pageTable(TwoLevelPageTable(pid)), pid(pidGiven), size(0), heapPages(0), code(0), stack(0), heap(code) {}

void process::allocateMem(uint32_t allocatedSize) {
    heapPages++;
    heap += allocatedSize;
    size += allocatedSize;
}

void process::freeMem(uint32_t freedSize) {
    heapPages--;
    heap -= freedSize;
    size -= freedSize;
}

// free  input: size
// pagetable per process
// every time os allocate memory, 
// 从右往左便利vpnlist 然后free需要的size
// invalidate pt

//
