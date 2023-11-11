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
    TwoLevelPageTable& pageTable;
    std::vector<bool> memoryMap;
    std::vector<process> processes;
    uint32_t high_watermark;
    uint32_t low_watermark;
    int pagesize;
    size_t totalFreePages;
    std::vector<uint32_t> disk;
    std::map<uint32_t, size_t> pageToDiskMap;

public:
    os(TwoLevelPageTable& pt, size_t memorySize, size_t diskSize, uint32_t high_watermarkGiven, uint32_t low_watermarkGiven);

    uint32_t allocateMemory(process& proc, size_t size);
    void freeMemory(long int baseAddress, size_t memorySizeToFree);
    uint32_t createProcess(long int pid);
    void destroyProcess(long int pid);
    void handleTLBMiss();
    void swapOutToMeetWatermark();
    void swapOutPage(uint32_t vpn);
    void swapInPage(uint32_t vpn);
    size_t findFreeFrame();
};

#endif // OS_H
