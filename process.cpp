// process.cpp
#include "process.h"
#include <iostream>

using namespace std;

process::process(long int pidGiven) : pid(pidGiven), size(0), code(0), stack(0), heap(code) {}

void process::allocateMem(uint32_t allocatedSize) {
    heap += allocatedSize;
    size += allocatedSize;
}

long int process::getPid() const {
    return pid;
}

long int process::getSize() const {
    return size;
}

long int process::getCode() const {
    return code;
}

long int process::getStack() const {
    return stack;
}

long int process::getHeap() const {
    return heap;
}
