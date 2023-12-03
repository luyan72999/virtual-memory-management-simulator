#ifndef OS_H
#define OS_H

#include "TwoLevelPageTable.h"
#include "process.h"
#include "tlb.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <stdexcept>
using namespace std;

extern int memory_access_attempts;
extern int stack_miss;
extern int heap_miss;
extern int code_miss;

class os {
private:
    int minPageSize;
    process* runningProc;
    vector<bool> memoryMap;
    vector<process> processes;
    vector<bool> diskMap;
    uint32_t high_watermark;
    uint32_t low_watermark;
    uint32_t totalFreeSize;
    //std::vector<uint32_t> disk;
    map<uint32_t, uint32_t> pageToDiskMap;
    Tlb tlb;


public:
    os(size_t memorySize, size_t diskSize, uint32_t high_watermarkGiven, uint32_t low_watermarkGiven);
    ~os();

    uint32_t allocateMemory(uint32_t size);
    void freeMemory(uint32_t baseAddress);
    uint32_t createProcess(long int pid);
    //void destroyProcess(long int pid);
    void swapOutToMeetWatermark(uint32_t sizeTobeFree);
    void swapOutPage(uint32_t vpn, uint32_t pfn);
    uint32_t swapInPage(uint32_t vpn, uint32_t size);
    uint32_t findFreeFrame();
    void handleInstruction(const string& string, uint32_t value, uint32_t pid);
    uint32_t accessStack(uint32_t baseAddress);
    uint32_t accessHeap(uint32_t baseAddress);
    uint32_t accessCode(uint32_t baseAddress);
    uint32_t accessMemory(uint32_t baseAddress);
    void switchToProcess(uint32_t pid);
    vector<pair<uint32_t, uint32_t> > findPhysicalFrames(uint32_t size);
    uint32_t findFreeDiskBlock();
};

#endif // OS_H
