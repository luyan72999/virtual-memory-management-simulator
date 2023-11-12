#include "TwoLevelPageTable.h"
#include "process.h"
#include "os.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdint.h> 
#include <random>
#include <ctime>
#include <stdexcept>
#include <cstdint>
#include <map>


using namespace std;


os::os(TwoLevelPageTable& pt, size_t memorySize, size_t diskSize, uint32_t high_watermarkGiven, uint32_t low_watermarkGiven) 
  : pageTable(pt), memoryMap(memorySize, false), disk(diskSize, 0), high_watermark(high_watermarkGiven), low_watermark(low_watermarkGiven), pagesize(4096), totalFreeSize(2^32) {
}

uint32_t os::allocateMemory(process& proc, uint32_t size) {
    size_t pagesNeeded = ceil((size) / pagesize);

    if (totalFreeSize - size < low_watermark) {
        // Swap out pages to maintain free memory above the low watermark
        uint32_t sizeTobeFree = high_watermark - (totalFreeSize - size);
        swapOutToMeetWatermark(sizeTobeFree);
    }

    size_t freePages = 0;
    size_t start = 0;

    for (size_t i = 0; i < memoryMap.size(); ++i) {
        if (!memoryMap[i]) { // If the page is free
            if (freePages == 0) start = i;
            freePages++;
            if (freePages == pagesNeeded) {
                size_t startAddress = proc.getHeap();
                proc.allocateMem(size);  // Update the process's heap top

                for (size_t j = start; j < start + pagesNeeded; ++j) {
                    memoryMap[j] = true;  // Mark pages as allocated
                    uint32_t pfn = j;
                    uint32_t vpn = startAddress / pagesize + (j - start);
                    // uint32_t pdi = vpn >> pteIndexBits;

                    // what should pageDirectoryBaseAddress be
                    //uint32_t pdeAddress = pageDirectoryBaseAddress + (pdi * sizeOfPDE);
                    pageTable.setMapping(size, vpn, pfn);
                }
                return startAddress;
            }
        } else {
            freePages = 0;
        }
    }
    throw runtime_error("Not enough memory to allocate");
}

void os::freeMemory(uint32_t baseAddress, uint32_t memorySizeToFree) {
    uint32_t startVpn = baseAddress / pagesize;
    size_t pagesToFree = ceil((memorySizeToFree) / pagesize);

    for (size_t i = 0; i < pagesToFree; i++) {
        uint32_t currentVpn = startVpn + i;

        // Invalidate the page table entry for this VPN
        pageTable.free(currentVpn);

        // Free the physical memory, if present
        size_t pfn = pageTable.translate(currentVpn);
        if (pfn < memoryMap.size()) {
            memoryMap[pfn] = false; // Mark the physical page as free
        }

        // Free from disk if it's swapped out
        map<uint32_t, size_t>::iterator res = pageToDiskMap.find(currentVpn);
        if (res != pageToDiskMap.end()) {
            size_t diskIndex = res->second;
            disk[diskIndex] = 0; // Clear the disk space
            pageToDiskMap.erase(res); // Remove the entry from the map
        }

        // Invalidate TLB entry for this VPN
    }
}

uint32_t os::createProcess(long int pid) {
    process newProcess(pid);
    processes.push_back(newProcess);
    return pid;
}

void os::destroyProcess(long int pid) {
    for (int i = 0; i < processes.size(); i++) {
      if (processes[i].getPid() == pid) {
        freeMemory(processes[i].getHeap(), processes[i].getSize());
        processes.erase(processes.begin() + i);
        return;
      }
    }
    throw runtime_error("Process with PID " + to_string(pid) + " not found.");
}

void os::swapOutToMeetWatermark(uint32_t sizeToFree) {
    size_t freedMemory = 0;
    uint32_t pfnBits = 20;

    for (process& proc : processes) {
        if (freedMemory >= sizeToFree) break; 

        uint32_t currentAddress = proc.getCode(); // Start from the beginning
        uint32_t endAddress = proc.getHeap();

        while (currentAddress < endAddress && freedMemory < sizeToFree) {         
            pair<uint32_t, uint32_t> pteAndPageSize = pageTable.translate(currentAddress);
            uint32_t pageSize = pteAndPageSize.second;
            uint32_t pte = pteAndPageSize.first;
            uint32_t vpn = currentAddress / pageSize;
            uint32_t pfn = pte & ((1 << pfnBits) - 1);

            swapOutPage(vpn, pfn); // Call swapOutPage for the calculated VPN

            freedMemory += pageSize;
            currentAddress += pageSize; // Move to the next page
        }
    }
}

void os::swapOutPage(uint32_t vpn, uint32_t pfnToSwapOut) {
    if (pfnToSwapOut < memoryMap.size() && memoryMap[pfnToSwapOut]) {
        disk.push_back(pfnToSwapOut); // Store the page data on the disk
        memoryMap[pfnToSwapOut] = false; // Free the page in physical memory

        // Update the map to reflect where the page is stored on disk
        pageToDiskMap[vpn] = disk.size() - 1;

        // update present bit
        // pt.update(vpn)
    }
} 


/*
void handleTLBMiss(uint32_t virtualAddress) {
    //map = pt.getmapToPDEs();
      uint32_t pde = mapToPDEs[vpn];
      int validBitPDE = pde >> pfnBits;
      int presentBitPDE = pde >> (pfnBits + 1);

      if (presentBitPDE == 0) {
        swapInPage(vpn);
      }
}
*/

uint32_t os::swapInPage(uint32_t vpn) {
    // Check if the page is on disk
    map<uint32_t, size_t>::iterator res = pageToDiskMap.find(vpn);
    if (res != pageToDiskMap.end()) {
      size_t freePfn = findFreeFrame();
      if (freePfn == -1) {
          throw runtime_error("No free frame available in physical memory");
      }

      // Clear the entry on the disk and remove it from pageToDiskMap
      disk[res->second] = 0;
      pageToDiskMap.erase(res);

      //update total free size

      return freePfn;
    } else {
        throw runtime_error("Page not found on disk for VPN " + to_string(vpn));
    }
}

size_t os::findFreeFrame() {
    for (size_t i = 0; i < memoryMap.size(); ++i) {
        if (!memoryMap[i]) {
            return i; // Free frame found
        }
    }
    return -1; // No free frame found
}