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
  : pageTable(pt), memoryMap(memorySize, false), disk(diskSize, 0), high_watermark(high_watermarkGiven), low_watermark(low_watermarkGiven), pagesize(4096), totalFreePages(memoryMap.size()) {
}

uint32_t os::allocateMemory(process& proc, size_t size) {
    size_t pagesNeeded = ceil((size) / pagesize);

    //not sure if seeting total freepages correct
    if (totalFreePages - pagesNeeded < low_watermark) {
        // Swap out pages to maintain free memory above the low watermark
        swapOutToMeetWatermark();
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
                    long int pfn = j;
                    long int vpn = startAddress / pagesize + (j - start);
                    pageTable.setMapping(vpn, pfn, pfn);
                }
                return startAddress;
            }
        } else {
            freePages = 0;
        }
    }
    throw runtime_error("Not enough memory to allocate");
}

void os::freeMemory(long int baseAddress, size_t memorySizeToFree) {
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

void os::handleTLBMiss() {
    // Method implementation
    // ...
}

void os::swapOutToMeetWatermark() {
    size_t pagesToFree = high_watermark - totalFreePages;

    for (process& proc : processes) {
      if (pagesToFree == 0) break; // Stop if we've freed enough pages

      uint32_t startAddress = proc.getCode();
      uint32_t endAddress = proc.getHeap();
      uint32_t startVpn = startAddress / pagesize;
      uint32_t endVpn = (endAddress + pagesize - 1) / pagesize; // Include partial pages

      for (uint32_t vpn = startVpn; vpn < endVpn && pagesToFree > 0; vpn++) {
          swapOutPage(vpn);
          pagesToFree--;
      }
    }
}

void os::swapOutPage(uint32_t vpn) {
    // Translate VPN to PFN
    size_t pfnToSwapOut = pageTable.translate(vpn);

    if (pfnToSwapOut < memoryMap.size() && memoryMap[pfnToSwapOut]) {
      // Store the page data on the disk using PFN
      disk.push_back(pfnToSwapOut); 

      // Update the map to reflect where the page is stored on disk
      pageToDiskMap[vpn] = disk.size() - 1;

      // Free the page in physical memory
      memoryMap[pfnToSwapOut] = false;

      // Invalidate the page table and TLB entries
      pageTable.free(vpn);


      // handle TLB invalidation
    }
}

void os::swapInPage(uint32_t vpn) {
    // Check if the page is on disk
    map<uint32_t, size_t>::iterator res = pageToDiskMap.find(vpn);
    if (res != pageToDiskMap.end()) {
      size_t freePfn = findFreeFrame();
      if (freePfn == -1) {
          throw runtime_error("No free frame available in physical memory");
      }

      // Move the page from disk to the free frame in physical memory
      memoryMap[freePfn] = true;

      // Update the page table with the new mapping
      pageTable.setMapping(vpn, freePfn, freePfn);

      // Clear the entry on the disk and remove it from pageToDiskMap
      disk[res->second] = 0;
      pageToDiskMap.erase(res);

      totalFreePages++;
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