#ifndef PROCESS_H
#define PROCESS_H

#include <cstdint>

class process {
private:
    long int pid;
    long int size;
    uint32_t code;
    uint32_t stack;
    uint32_t heap;

public:
    process(long int pidGiven);

    void allocateMem(uint32_t allocatedSize);

    long int getPid() const;
    long int getSize() const;
    long int getCode() const;
    long int getStack() const;
    long int getHeap() const;
};

#endif