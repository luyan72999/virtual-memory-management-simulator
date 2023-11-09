#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>


using namespace std;


/**
 * Create a two-level page table for a process, given the page size, the PDBR and the memory vector.
 * Given a vpn, ptePfn and pfn, set a mapping from virtual page to physical frame.
 * Given a vpn, translate it to a pfn.
 * Physical memory 16G
 * Address space 32bit
 * page size from 4KB to 1GB
 */
class TwoLevelPageTable {
private:
    long long int physMem = pow(2, 34);
    int physPageSize = 4096;//physical page size is always 4KB
    long int physPageNumber = physMem / physPageSize;// 2^34/2^12=2^22
    int pfnBits = log2(physPageNumber);//22
    long int pagesNeeded;//multiple PTEs point to one pfn
    long int numOfPagesForPageTable;//number of pages for table

    long long int virtualMem = pow(2, 32);
    long int virtualPageNumber;
    int vpnBits;

    int flagBits = 1;
    int pdeSize = ceil((flagBits + pfnBits) / 8.0);// 3
    int pteSize = ceil((flagBits + pfnBits) / 8.0);// 3

    int pdeNumberOfBits;
    int pdeShift;
    long int pdeMask;

    long int pteEntryPerPage;
    int pteNumberOfBits;
    long int pteMask;
    int pteShift;

    long int pdbr;
    long int pageSize;
    vector<long int>& memory;

public:
    TwoLevelPageTable(long int pageSizeGiven, long int pdbrGiven, vector<long int> &memoryGiven) : memory(memoryGiven) {
        pageSize = pageSizeGiven; //8192
        pdbr = pdbrGiven; //108
        pagesNeeded = pageSize / physPageSize;// 2

        virtualPageNumber = virtualMem / pageSize;// 2^32/2^13=2^19
        vpnBits = log2(virtualPageNumber);// 19
        numOfPagesForPageTable = ceil(virtualPageNumber * pteSize * 1.0 / pageSize);// 192

        int temp = floor(log2(pageSize / pteSize));// 11
        if (pow(2, temp) < virtualPageNumber) {
            pteEntryPerPage = pow(2, temp);
        } else {
            pteEntryPerPage = virtualPageNumber;
        }
        pteNumberOfBits = ceil(log2(pteEntryPerPage));//11
        pdeNumberOfBits = vpnBits - pteNumberOfBits;// 8

        pdeShift = pteNumberOfBits;// 11
        pdeMask = ((1 << pdeNumberOfBits) - 1) << pdeShift;// 0111 1111 1000 0000 0000

        pteMask = (1 << pteNumberOfBits) - 1;// 0000 0000 0111 1111 1111
        pteShift = pteNumberOfBits;// 11
    }

    long int getPageSize() const {
        return pageSize;
    }
    // set a mapping from virtual page to physical frame
    void setMapping(long int vpn, long int ptePfn, long int pfn) {
        //pdbr 1101100
        //pfn  1010101
        //ptePfn 100001
        //vpn     0000 1011 1101 1100 0000
        //pdeMask 0111 1111 1000 0000 0000
        //vpn & pdeMask -> 0000 1011 1000 0000 0000
        long int pdeBits = (vpn & pdeMask) >> pdeShift; // 10111
        //pdbr<<pdeShift    0011 0110 0000 0000 0000
        //pdeBits           0000 0000 0000 0001 0111
        //pdeBits * pdeSize 0000 0000 0000 0100 0101
        long int pdeAdr = (pdbr << pdeShift) | (pdeBits * pdeSize); // 0011 0110 0000 0100 0101, 221253
        long int pde = memory[pdeAdr];
        int validPDE = pde >> pfnBits;
        if (validPDE == 0) {
            memory[pdeAdr] = (1 << pfnBits) | ptePfn; // set valid to 1 and fill pointer to pte page
        } else {
            cout << "PDE fault." << endl;
        }

        //pteMask 0000 0000 0111 1111 1111
        //vpn     0000 1011 1101 1100 0000
        long int pteBits = vpn & pteMask; // 0000 0000 0101 1100 0000
        //ptePfn<<pteShift        0001 0000 1000 0000 0000
        //pteBits                 0000 0000 0101 1100 0000
        //pteBits * pteSize       0000 0001 0001 0100 0000
        //pteAdr                  0001 0001 1001 0100 0000
        long int pteAdr = (ptePfn << pteShift) | (pteBits * pteSize);
        long int pte = memory[pteAdr];
        int validPTE = pte >> pfnBits;
        if (validPTE == 0) {
            for (long int i = pteAdr; i < (pteAdr + pagesNeeded * pteSize);) {
                memory[i] = (1 << pfnBits) | pfn; // set valid to 1 and fill pointer to page frame number
                i += pteSize;
            }
        } else {
            cout << "PTE fault." << endl;
        }
    }

    // Function to get the physical frame from virtual page
    long int translate(long int vpn) {
        //pdbr 1101100
        //pfn  1010101
        //ptePfn 100001
        //vpn     0101 1010 0101 1101 1010
        //pdeMask 1111 1111 1000 0000 0000
        //vpn & pdeMask -> 0101 1010 0000 0000 0000
        long int pdeBits = (vpn & pdeMask) >> pdeShift; //10110100
        //pdbr<<pdeShift   0011 0110 0000 0000 0000
        //pdeBits          0000 0000 0000 1011 0100
        long int pdeAdr = (pdbr << pdeShift) | (pdeBits * pdeSize); // 0011 0110 0000 1011 0100, 221364
        long int pde = memory[pdeAdr]; // 10100001
        int validPDE = pde >> pfnBits;
        if (validPDE == 1) {
            long int ptePfn = pde & ((1 << pfnBits) - 1); //100001
            //pteMask 0000 0000 0111 1111 1111
            //vpn     0101 1010 0101 1101 1010
            long int pteBits = vpn & pteMask; // 0000 0000 0101 1101 1010
            //ptePfn<<pteShift        0001 0000 1000 0000 0000
            //pteBits                 0000 0000 0101 1101 1010
            //pteAdr                  0001 0000 1101 1101 1010, 69082
            long int pteAdr = (ptePfn << pteShift) | (pteBits * pteSize);
            long int pte = memory[pteAdr];
            int validPTE = pte >> pfnBits;
            if (validPTE == 1) {
                long int pfn = memory[pteAdr] = pte & ((1 << pfnBits) - 1);
                return pfn;
            } else {
                cout << "PTE valid bit is 0." << endl;
            }
        } else {
            cout << "PDE valid bit is 0." << endl;
        }
    }

    // Function to remove the page table for the process
    void free() {
        long int pdeStartAdr = pdbr << pdeShift;
        for (long int i = 0; i < numOfPagesForPageTable;) {
            long int pde = memory[pdeStartAdr + i * pdeSize];
            long int pteStartAdr = (pde & ((1 << pfnBits) - 1)) << pteShift;
            for (long int j = pteStartAdr; j < pteStartAdr + pteEntryPerPage * pteSize; j++) {
                memory[j] = 0;
            }
            i += pdeSize;
        }
        for (long int i = pdeStartAdr; i < pdeStartAdr + numOfPagesForPageTable * pdeSize; i++) { memory[i] = 0; }
    }
};


int main() {

    long int pageSize = pow(2, 13);
    long int pdbr = 108;
    long int memorySize = 1000000;
    vector<long int> memory(memorySize, 0); // cannot support a too large vector

    TwoLevelPageTable pageTable(pageSize, pdbr, memory);

    // Setting some mappings
    pageTable.setMapping(0xBDC0, 0x21, 0x35); //48675, 33, 55

    // Getting some mappings
    long int frame = pageTable.translate(0xBDC0);

    // Printing results
    cout << "Mapping for virtual page 0xBDC0: Physical frame " << frame << endl;

    //vpn 0000 1011 1101 1100 0000
    long int checkAdr = (pdbr << 11) | (0b10111 * 3);
    cout << "There should be something: " << memory[checkAdr] << endl;

    pageTable.free();
    //check if successfully free the page table
    cout << "Here should be 0: " << memory[checkAdr] << endl;

    return 0;
}

class process {
private:
    long int pid;
    long int size;
    uint32_t heapTop;

public:
    process(long int pidGiven, long int sizeGiven) {
        pid = pidGiven;
        size = sizeGiven;
        heapTop = 0;
    }

    long int getPid() const {
        return pid;
    }

    long int getSize() const {
        return size;
    }

    uint32_t getHeapTop() const {
        return heapTop;
    }

    void updateHeapTop(long int size) {
        heapTop += size;
    }
};


class os {
private:
    TwoLevelPageTable& pageTable;
    vector<bool> memoryMap; // Represent physical address. True for allocated, false for free

public:
    os(TwoLevelPageTable& pt, size_t memorySize) 
        : pageTable(pt), memoryMap(memorySize, false) {}

    long int allocateMemory(process& process, long int size) {
        size_t pagesNeeded = ceil(static_cast<double>(size) / pageTable.getPageSize());
        size_t freePages = 0;
        size_t start = 0;

        for (size_t i = 0; i < memoryMap.size(); ++i) {
            if (!memoryMap[i]) { // If the page is free
                if (freePages == 0) start = i; // Mark start of potential free block
                freePages++;
                if (freePages == pagesNeeded) {
                    for (size_t j = start; j < start + pagesNeeded; ++j) {
                        memoryMap[j] = true; // Mark pages as allocated
                        long int pfn = j; // Virtual page number
                        long int vpn = process.getHeapTop() / pageTable.getPageSize(); //process.get // Allocate a physical frame
                        pageTable.setMapping(vpn, pfn, pfn); // Set the mapping for each page
                    }
                    return start * pageTable.getPageSize(); // Return starting virtual address of allocated block
                }
            } else {
                freePages = 0; // Reset the count if a used page is encountered
            }
        }
        throw runtime_error("Not enough memory to allocate");
    }

    void freeMemory(long int startAddress, long int size) {
        size_t pagesToFree = size / pageTable.getPageSize();
        size_t startIndex = startAddress / pageTable.getPageSize();

        if (startIndex + pagesToFree > memoryMap.size()) {
            throw runtime_error("Invalid memory block to free");
        }

        for (size_t i = startIndex; i < startIndex + pagesToFree; ++i) {
            memoryMap[i] = false; // Mark pages as free
            // Also invalidate the page table entry for this page (pagetable.remove/invalidate(vpn))
        }
    }
};

int main() {

    long int pageSize = pow(2, 12);
    long int pdbr = 108;
    long int memorySize = 10000;
    vector<long int> memory(memorySize, 0); // cannot support a too large vector

    TwoLevelPageTable pageTable(pageSize, pdbr, memory);

    process p1(1, 4096);

    os operatingSystem(pageTable, memorySize);

    long int allocatedAddress = operatingSystem.allocateMemory(p1, 4096);

    operatingSystem.freeMemory(allocatedAddress, 4096);

    // Setting some mappings
    pageTable.setMapping(0x6, 0x21, 0x35); //6, 33, 55

    // Getting some mappings
    long int frame = pageTable.translate(0x6);

    // Printing results
    cout << "Mapping for virtual page 0x6: Physical frame " << frame << endl;

    return 0;
}
