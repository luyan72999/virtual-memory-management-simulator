#ifndef OS_H
#define OS_H

#include "TwoLevelPageTable.h"
#include "process.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <stdexcept>

class os {
private:
    int minPageSize;
    process* runningProc;
    std::vector<bool> memoryMap;
    std::vector<process> processes;
    uint32_t high_watermark;
    uint32_t low_watermark;
    uint32_t totalFreeSize;
    std::vector<uint32_t> disk;
    std::map<uint32_t, size_t> pageToDiskMap;

public:
    os(size_t memorySize, size_t diskSize, uint32_t high_watermarkGiven, uint32_t low_watermarkGiven);

    uint32_t allocateMemory(uint32_t size);
    void freeMemory(uint32_t baseAddress);
    uint32_t createProcess(long int pid);
    void destroyProcess(long int pid);
    void swapOutToMeetWatermark(uint32_t sizeTobeFree);
    void swapOutPage(uint32_t vpn, uint32_t pfn);
    uint32_t swapInPage(uint32_t vpn, uint32_t size);
    size_t findFreeFrame();
    void handleInstruction(const string& string, uint32_t value, uint32_t pid);
    uint32_t accessStack(uint32_t baseAddress);
    uint32_t accessHeap(uint32_t baseAddress);
    uint32_t accessCode(uint32_t baseAddress);
    void switchToProcess(uint32_t pid);
};

#endif // OS_H
