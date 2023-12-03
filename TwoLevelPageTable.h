// TwoLevelPageTable.h

#ifndef TWO_LEVEL_PAGE_TABLE_H
#define TWO_LEVEL_PAGE_TABLE_H

#include <iostream>
#include <vector>
#include <cstdint>
#include <map>

using namespace std;

/**
 * Create a two-level page table for a process, given the PID.
 * Given page size, vpn, and pfn, set a mapping from virtual page to physical frame.
 * Given a vpn, translate it to a pfn, return pte.
 * Physical memory 32bit
 * Address space 32bit
 * page size from 4KB to 1GB
 */

extern int memory_hit;

struct PTE {
    uint32_t vpn;
    uint32_t pfn;
    uint32_t page_size;
    bool present;
    bool valid;
    PTE(uint32_t vpn, uint32_t pfn, uint32_t page_size);
    PTE();
};


class TwoLevelPageTable {
private:
    uint32_t pid;
    int physMemBits = 32;
    int virtualMemBits = 32;
    int pfnBits = physMemBits - 12;
    map<uint32_t, map<uint32_t, PTE>> mapToPDEs;

public:
    TwoLevelPageTable(int pidGiven);

    void setMapping(uint32_t pageSize, uint32_t vpn, uint32_t pfn);

    PTE translate(uint32_t vpn);

    void free(uint32_t vpn);
    void updatePresentBit(uint32_t vpn);
};

#endif // TWO_LEVEL_PAGE_TABLE_H
