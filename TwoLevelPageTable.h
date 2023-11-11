// TwoLevelPageTable.h

#ifndef TWO_LEVEL_PAGE_TABLE_H
#define TWO_LEVEL_PAGE_TABLE_H

#include <iostream>
#include <vector>
#include <cstdint>
#include <map>

using namespace std;

class TwoLevelPageTable {
private:
    uint32_t pid;
    int physMemBits = 32;
    int virtualMemBits = 32;
    int pfnBits = physMemBits - 12;
    map<uint32_t, uint32_t> mapToPDEs;
    map<uint32_t, uint32_t> mapToPTEs;
    map<uint32_t, uint32_t> vpnAndPageSize;

public:
    TwoLevelPageTable(int pidGiven);

    void setMapping(uint32_t pageSize, uint32_t vpn, uint32_t ptePfn, uint32_t pfn);

    uint32_t translate(uint32_t vpn);

    void free(uint32_t vpn);
};

#endif // TWO_LEVEL_PAGE_TABLE_H
