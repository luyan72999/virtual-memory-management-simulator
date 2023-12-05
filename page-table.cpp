#include <iostream>
#include <vector>
#include <cstdint>
#include <map>
#include "TwoLevelPageTable.h"


using namespace std;

PTE::PTE(uint32_t vpn, uint32_t pfn, uint32_t page_size): vpn(vpn), pfn(pfn), page_size(page_size),
    present(true), valid(true) {}

PTE::PTE(): present(false), valid(false) {}

int memory_hit = 0;
const int pdeOffset = 10;   // assuming VPN is 20 bits and PDE & PTE index are 10 bits
const uint32_t tenBitsMask = 0b1111111111;
const uint32_t minPageSize = 4096;

// 1. constructor
//    input: pid
//    initialize page table，mapToPDEs
TwoLevelPageTable::TwoLevelPageTable(int pidGiven) {
    pid = pidGiven;
    for (uint32_t vpnPdeBits = 0; vpnPdeBits <= 0b1111111111; vpnPdeBits++) {
        mapToPDEs[vpnPdeBits] = map<uint32_t, PTE>();
    }
}


// 2. setMapping
//    input: pageSize, vpn, pfn
void TwoLevelPageTable::setMapping(uint32_t pageSize, uint32_t vpn, uint32_t pfn) {
    uint32_t numPTEs = pageSize / minPageSize;
    uint32_t numPDEs = numPTEs / 1024 + (numPTEs % 1024 == 0? 0: 1);
    uint32_t pdeIdx = vpn >> pdeOffset;

    for (uint32_t i = pdeIdx; i < pdeIdx + numPDEs; i++) {
        auto& mapToPte = mapToPDEs[pdeIdx]; //presentBit, validBit, ptePfn
        uint32_t pteIdx = vpn & tenBitsMask;
        for (uint32_t j = pteIdx; j < pteIdx + numPTEs; j++) {
                mapToPte.insert_or_assign(j, PTE(vpn, pfn, pageSize));
        }
    }
}

// 3. translate
//    input: virtual address
//    output: pte
PTE TwoLevelPageTable::translate(uint32_t vaddr) {
    uint32_t vpn = vaddr >> 12;
    auto& mapToPte = mapToPDEs[vpn >> pdeOffset];

    uint32_t pteIdx = vpn & tenBitsMask;
    PTE pte = mapToPte[pteIdx];
    memory_hit += 2;

    if (!pte.valid) {
        throw runtime_error("Valid bit of pte is 0.");
    }
    if (!pte.present) {
        // TODO: Call OS swap in method
        // Done
//        os.swapInPage(vpn, pte.page_size);
//        pte = mapToPte[pteIdx];
//        memory_hit += 2;
        //throw runtime_error("Present bit of pte is 0.");
        throw logic_error("Present bit of pte is 0.");
    }
    return pte;
}

// 4. free
//    remove mapping given vpn
void TwoLevelPageTable::free(uint32_t vpn) {
    uint32_t firstPteIdx = vpn & tenBitsMask;
    auto first = mapToPDEs[vpn >> pdeOffset][firstPteIdx];
    if (!first.valid) {
        return;
    }
    uint32_t numPTEs = first.page_size / minPageSize;
    uint32_t numPDEs = numPTEs / 1024 + (numPTEs % 1024 == 0? 0: 1);
    uint32_t pdeIdx = vpn >> pdeOffset;

    for (uint32_t i = pdeIdx; i < pdeIdx + numPDEs; i++) {
        auto& mapToPte = mapToPDEs[pdeIdx]; //presentBit, validBit, ptePfn
        uint32_t pteIdx = vpn & tenBitsMask;
        for (uint32_t j = pteIdx; j < pteIdx + numPTEs; j++) {
            mapToPte.erase(j);
        }
    }
}

//5.update present bit when swap out
void TwoLevelPageTable::updatePresentBit(uint32_t vpn) {
    uint32_t firstPteIdx = vpn & tenBitsMask;
    auto first = mapToPDEs[vpn >> pdeOffset][firstPteIdx];
    uint32_t numPTEs = first.page_size / minPageSize;
    uint32_t numPDEs = numPTEs / 1024 + (numPTEs % 1024 == 0? 0: 1);
    uint32_t pdeIdx = vpn >> pdeOffset;

    for (uint32_t i = pdeIdx; i < pdeIdx + numPDEs; i++) {
        auto& mapToPte = mapToPDEs[pdeIdx]; //presentBit, validBit, ptePfn
        uint32_t pteIdx = vpn & tenBitsMask;
        for (uint32_t j = pteIdx; j < pteIdx + numPTEs; j++) {
            mapToPte[j].present = false;
        }
    }
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
//}