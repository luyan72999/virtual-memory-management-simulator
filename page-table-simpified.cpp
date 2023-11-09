#include <iostream>
#include <vector>
#include <cstdint>
#include <map>

using namespace std;


/**
 * Create a two-level page table for a process, given the PID.
 * Given page size, vpn, ptePfn and pfn, set a mapping from virtual page to physical frame.
 * Given a vpn, translate it to a pfn, return the pfn, pageSize.
 * Physical memory 32bit
 * Address space 32bit
 * page size from 4KB to 1GB
 */
class TwoLevelPageTable {
private:
    uint32_t pid;
    int physMemBits=32;
    int virtualMemBits = 32;
    int pfnBits = physMemBits-12; //physical page size is always 4KB
    map<uint32_t, uint32_t> mapToPDEs;
    map<uint32_t, uint32_t> mapToPTEs;

public:
    // 1. constructor
    //    input: pid
    //    初始化page table，vectorOfPDEs, vectorOfPTEs
    TwoLevelPageTable(int pidGiven) {
    pid=pidGiven;
    }

    // 2. setMapping
    //    input: pageSize, vpn, ptePfn, pfn
    //    use vpn to get pde index (use some simplified rule?), vectorOfPDEs[pdeIndex] saves valid bit, present bit and ptePfn
    //    use ptePfn to get pte index (use some simplified rule?), vectorOfPTEs[pteIndex] saves valid bit, present bit and pfn??
    void setMapping(uint32_t pageSize, uint32_t vpn, uint32_t ptePfn, uint32_t pfn) {
        int pageSizeBits = log2(pageSize);
        int vpnBits = virtualMemBits - pageSizeBits;

        uint32_t pde = mapToPDEs[vpn]; //presentBit, validBit, ptePfn
        int validBitPDE = pde >> pfnBits;
        int presentBitPDE = pde>> (pfnBits+1);

        if(validBitPDE==1){
            cout<<"Seg fault."<<endl;
            return;
        }
        if(presentBitPDE==1){
            cout<<"Page fault."<<endl;
            return;
        }

        mapToPDEs[vpn] = (0b11<<pfnBits)|ptePfn;

        uint32_t pte = mapToPTEs[ptePfn]; //presentBit, validBit, pfn
        int validBitPTE = pte >> pfnBits;
        int presentBitPTE = pte>> (pfnBits+1);

        if(validBitPTE==1){
            cout<<"Seg fault."<<endl;
            return;
        }
        if(presentBitPTE==1){
            cout<<"Page fault."<<endl;
            return;
        }

        mapToPTEs[ptePfn] = (0b11<<pfnBits)|pfn;
    }

    // 3. translate
    //    input: vpn
    //    output: pfn
    uint32_t translate(uint32_t vpn) {
        uint32_t pde=mapToPDEs[vpn];
        int validBitPDE = pde >> pfnBits;
        int presentBitPDE = pde>> (pfnBits+1);

        if(validBitPDE==0){
            throw runtime_error("Valid bit of pde is 0.");
        }
        if(presentBitPDE==0){
            throw runtime_error("Present bit of pde is 0.");
        }

        uint32_t ptePfn=pde & ((1<<pfnBits)-1);
        uint32_t pte=mapToPTEs[ptePfn];
        int validBitPTE = pte >> pfnBits;
        int presentBitPTE = pte>> (pfnBits+1);

        if(validBitPTE==0){
            throw runtime_error("Valid bit of pte is 0.");
        }
        if(presentBitPTE==0){
            throw runtime_error("Present bit of pte is 0.");
        }

        uint32_t pfn=pte & ((1<<pfnBits)-1);
        return(pfn);
    }

    // 4. free
    //    remove mapping given vpn
    void free(long int vpn) {

}

};


int main() {
    int pid= 1;
    TwoLevelPageTable pageTable(pid);

    uint32_t pageSize = pow(2, 12);
    // Setting some mappings
    pageTable.setMapping(4096,0xBDC0, 0x21, 0x35); //48675, 33, 55

    // Getting some mappings
    long int frame = pageTable.translate(0xBDC0);
    cout << "Mapping for virtual page 0xBDC0: Physical frame " << frame << endl;

    return 0;
}