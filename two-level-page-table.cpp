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


    // 1. constructor
    //    input: pid
    //    初始化page table，vectorOfPDEs, vectorOfPTEs

    // 2. setMapping
    //    input: pageSize, vpn, ptePfn, pfn (举例，4kb的page size，vpn也还是给我20bits吗)
    //    use vpn to get pde index (use some simplified rule?), vectorOfPDEs[pdeIndex] saves valid bit, present bit and ptePfn
    //    use ptePfn to get pte index (use some simplified rule?), vectorOfPTEs[pteIndex] saves valid bit, present bit and pfn??
    //    如果是这样，这个page table是one-level还是two-level的有区别吗？
    //    而且我也看不出来，pageSize这里怎么用？

    // 3. translate
    //    input: vpn
    //    output: pfn, pageSize, pid

    // 4. free
    //    remove page table for the process
}
