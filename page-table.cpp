#include <iostream>
#include <vector>
#include <cstdint>
#include <map>
#include "TwoLevelPageTable.h"


using namespace std;


// 1. constructor
//    input: pid
//    initialize page table，vectorOfPDEs, vectorOfPTEs
TwoLevelPageTable::TwoLevelPageTable(int pidGiven) {
    pid = pidGiven;
}

// 2. setMapping
//    input: pageSize, vpn, pfn
//    use vpn to get pde index (use some simplified rule?), vectorOfPDEs[pdeIndex] saves valid bit, present bit and ptePfn
//    use ptePfn to get pte index (use some simplified rule?), vectorOfPTEs[pteIndex] saves valid bit, present bit and pfn??
void TwoLevelPageTable::setMapping(uint32_t pageSize, uint32_t vpn, uint32_t pfn) {
    vpnAndPageSize[vpn] = pageSize;
    int pageSizeBits = log2(pageSize);
    int vpnBits = virtualMemBits - pageSizeBits;
    uint32_t numPTEs = pageSize / 4096;

    uint32_t pde = mapToPDEs[vpn]; //presentBit, validBit, ptePfn
    int validBitPDE = pde >> pfnBits;
    int presentBitPDE = pde >> (pfnBits + 1);

    if (validBitPDE == 1) {
        cout << "Seg fault." << endl;
        return;
    }
    if (presentBitPDE == 1) {
        cout << "Page fault." << endl;
        return;
    }

    for(uint32_t i = 0; i < numPTEs; i++){

    }
    uint32_t ptePfn = vpn;
    mapToPDEs[vpn] = (0b11 << pfnBits) | ptePfn;

    for (uint32_t i = ptePfn; i < ptePfn + numPTEs; i++) {
        uint32_t pte = mapToPTEs[ptePfn]; //presentBit, validBit, pfn, page size
        int validBitPTE = pte >> pfnBits;
        int presentBitPTE = pte >> (pfnBits + 1);

        if (validBitPTE == 1) {
            cout << "Seg fault." << endl;
            return;
        }
        if (presentBitPTE == 1) {
            cout << "Page fault." << endl;
            return;
        }

        mapToPTEs[ptePfn] = (0b11 << pfnBits) | pfn;
    }
}

// 3. translate
//    input: virtual address
//    output: pte, page size
pair<uint32_t,uint32_t> TwoLevelPageTable::translate(uint32_t vaddr) {
    uint32_t vpn = vaddr >> 12;
    uint32_t pde = mapToPDEs[vpn];
    int validBitPDE = pde >> pfnBits;
    int presentBitPDE = pde >> (pfnBits + 1);

    if (validBitPDE == 0) {
        throw runtime_error("Valid bit of pde is 0.");
    }
    if (presentBitPDE == 0) {
        throw runtime_error("Present bit of pde is 0.");
    }

    uint32_t ptePfn = pde & ((1 << pfnBits) - 1);
    uint32_t pte = mapToPTEs[ptePfn];
    int validBitPTE = pte >> pfnBits;
    int presentBitPTE = pte >> (pfnBits + 1);

    if (validBitPTE == 0) {
        throw runtime_error("Valid bit of pte is 0.");
    }
    if (presentBitPTE == 0) {
        throw runtime_error("Present bit of pte is 0.");
    }
    return make_pair(pte, vpnAndPageSize[vpn]);

    //uint32_t pfn = pte & ((1 << pfnBits) - 1);
    //return (pfn);
}

// 4. free
//    remove mapping given vpn
void TwoLevelPageTable::free(uint32_t vpn) {
    uint32_t pageSize = vpnAndPageSize[vpn];
    uint32_t numPTEs = pageSize / 4096;
    uint32_t pde = mapToPDEs[vpn];
    int validBitPDE = pde >> pfnBits;
    int presentBitPDE = pde >> (pfnBits + 1);

    if (validBitPDE == 0) {
        throw runtime_error("Valid bit of pde is 0. Nothing to free.");
    }
    if (presentBitPDE == 0) {
        throw runtime_error("Present bit of pde is 0.");
    }
    uint32_t ptePfn = pde & ((1 << pfnBits) - 1);

    for (uint32_t i = ptePfn; i < ptePfn + numPTEs; i++) {
        uint32_t pte = mapToPTEs[ptePfn];
        int validBitPTE = pte >> pfnBits;
        int presentBitPTE = pte >> (pfnBits + 1);

        if (validBitPTE == 0) {
            throw runtime_error("Valid bit of pte is 0. Nothing to free.");
        }
        if (presentBitPTE == 0) {
            throw runtime_error("Present bit of pte is 0.");
        }

        mapToPTEs.erase(ptePfn);
    }
    mapToPDEs.erase(vpn);
}


//for testing

//int main() {
//    int pid= 1;
//    TwoLevelPageTable pageTable(pid);
//
//    uint32_t pageSize = pow(2, 12);
//    // Setting some mappings
//    pageTable.setMapping(4096,0xBDC00, 0x35); //48675, 33, 55
//
//    // Getting some mappings
//    auto result = pageTable.translate(0xBDC0005A);
//    uint32_t pte=result.first;
//    uint32_t pageSizeGet=result.second;
//    cout << "Mapping for virtual page 0xBDC0: Physical frame " << (pte & (0b0011111111111111111111)) << endl;
//    cout << "Page size: " << pageSizeGet << endl;
//
//    return 0;
}