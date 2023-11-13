#include "TwoLevelPageTable.h"
#include "process.h"
#include "os.h"
#include "tlb.h"
#include <iostream>
#include <utility>
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


os::os(size_t memorySize, size_t diskSize, uint32_t high_watermarkGiven,
       uint32_t low_watermarkGiven)
    : minPageSize(4096), memoryMap(memorySize / minPageSize, false),
      high_watermark(high_watermarkGiven), low_watermark(low_watermarkGiven),
      totalFreeSize(-1), tlb(Tlb(64, 1024, 4)) {
}

os::~os() {
}


uint32_t os::allocateMemory(uint32_t size) {
    if (totalFreeSize - size < low_watermark) {
        // Swap out pages to maintain free memory above the low watermark
        uint32_t sizeTobeFree = high_watermark - (totalFreeSize - size);
        swapOutToMeetWatermark(sizeTobeFree);
    }

    auto frames = findPhysicalFrames(size);
    uint32_t vpn = (runningProc->heap) >> 12;   // 12 is 4k page's intra-page offset bits
    for (auto p : frames) {
        auto pfn = p.first;
        auto frame_size = p.second;
        runningProc->pageTable.setMapping(frame_size, vpn, pfn);
        vpn += frame_size / minPageSize;
    }
    runningProc->allocateMem(size);
}

void os::freeMemory(uint32_t baseAddress) {
    uint32_t sizeToFree = (runningProc->heap - baseAddress);
    uint32_t pagesToFree = sizeToFree / minPageSize;
    uint32_t sizeFreed = 0;
    uint32_t vpn = baseAddress >> 12;

    while (sizeFreed != sizeToFree) {
        auto p = runningProc->pageTable.translate(baseAddress);
        runningProc->pageTable.free(vpn);
        uint32_t basePfn = p.pfn, pageSize = p.page_size;
        for (uint32_t pfn = basePfn; pfn < basePfn + pageSize / minPageSize;
             pfn++) {
            memoryMap[pfn] = false;
        }
        vpn += pageSize >> 12;
        sizeFreed += pageSize;
        baseAddress += pageSize;
    }
    runningProc->freeMem(sizeToFree);

    // Invalidate TLB entry for this VPN
}

uint32_t os::createProcess(long int pid) {
    process newProcess(pid);

    uint32_t codeSize = 4 * 1024 * 1024;
    newProcess.code = codeSize - 1;
    newProcess.heap = codeSize;
    uint32_t code_vpn = 0;
    auto code_frames = findPhysicalFrames(codeSize);
    for (auto &p : code_frames) {
        uint32_t pfn = p.first;
        uint32_t size = p.second;
        newProcess.pageTable.setMapping(size, code_vpn, pfn);
        code_vpn += size / minPageSize;
    }

    uint32_t stackSize = 4 * 1024 * 1024;
    auto stack_frames = findPhysicalFrames(stackSize);
    newProcess.stack = 0xFFFFFFFF - stackSize + 1;
    uint32_t stack_vpn = newProcess.stack / minPageSize;
    for (auto &p : stack_frames) {
        uint32_t pfn = p.first;
        uint32_t size = p.second;
        newProcess.pageTable.setMapping(size, stack_vpn, pfn);
        stack_vpn += size / minPageSize;
    }
    processes.push_back(newProcess);

    return pid;
}

/*
void os::destroyProcess(long int pid) {
    for (int i = 0; i < processes.size(); i++) {
      if (processes[i].pid == pid) {
        freeMemory(processes[i].heap);
        processes.erase(processes.begin() + i);
        return;
      }
    }
    throw runtime_error("Process with PID " + to_string(pid) + " not found.");
}
*/

void os::swapOutToMeetWatermark(uint32_t sizeToFree) {
    size_t freedMemory = 0;
    uint32_t pfnBits = 20;

    for (process& proc : processes) {
        if (freedMemory >= sizeToFree) break; 

        uint32_t currentAddress = proc.code; // Start from the beginning
        uint32_t endAddress = proc.heap;

        while (currentAddress < endAddress && freedMemory < sizeToFree) {
            auto pteAndPageSize =
                runningProc->pageTable.translate(currentAddress);
            uint32_t pageSize = pteAndPageSize.page_size;
            uint32_t pfn = pteAndPageSize.pfn;
            uint32_t vpn = currentAddress / pageSize;

            swapOutPage(vpn, pfn); // Call swapOutPage for the calculated VPN

            freedMemory += pageSize;
            currentAddress += pageSize; // Move to the next page

            tlb.invalidate_tlb(proc.pid, vpn);
        }
    }
}

void os::swapOutPage(uint32_t vpn, uint32_t pfnToSwapOut) {
    if (pfnToSwapOut < memoryMap.size() && memoryMap[pfnToSwapOut]) {
        //disk.push_back(pfnToSwapOut); // Store the page data on the disk
        memoryMap[pfnToSwapOut] = false; // Free the page in physical memory

        // Update the map to reflect where the page is stored on disk
        //pageToDiskMap[vpn] = disk.size() - 1;

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

uint32_t os::swapInPage(uint32_t vpn, uint32_t size) {
    auto frames = findPhysicalFrames(size);
    for (auto p : frames) {
        auto pfn = p.first;
        auto size = p.second;
        runningProc->pageTable.setMapping(size, vpn, pfn);
        vpn += size / minPageSize;
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

void os::handleInstruction(const string& instruction, uint32_t value, uint32_t pid) {
    uint32_t result;
    if (instruction == "alloc") {
      result = allocateMemory(value);
    } else if (instruction == "free") {
      freeMemory(value);
    } else if (instruction == "access_stak") {
      result = accessStack(value);
    } else if (instruction == "access_heap") {
      result = accessHeap(value);
    } else if (instruction == "access_code") {
      result = accessCode(value);
    } else if (instruction == "switch") {
      switchToProcess(pid);
    }
}

uint32_t os::accessStack(uint32_t address) {
    try {
        auto addr = tlb.look_up(address, runningProc->pid);
        return addr;
    } catch (const exception& e) {
        auto pte = runningProc->pageTable.translate(address >> 12);
        auto tlbEntry = tlb.create_tlb_entry(pte.pfn, pte.page_size, address, runningProc->pid);
        tlb.l1_insert(tlbEntry);
        tlb.l2_insert(tlbEntry);
        auto addr = tlb.look_up(address, runningProc->pid);
        return addr;
    }
}

uint32_t os::accessHeap(uint32_t address) {
    try {
        auto addr = tlb.look_up(address, runningProc->pid);
        return addr;
    } catch (const exception& e) {
        auto pte = runningProc->pageTable.translate(address >> 12);
        auto tlbEntry = tlb.create_tlb_entry(pte.pfn, pte.page_size, address, runningProc->pid);
        tlb.l1_insert(tlbEntry);
        tlb.l2_insert(tlbEntry);
        auto addr = tlb.look_up(address, runningProc->pid);
        return addr;
    }
}

uint32_t os::accessCode(uint32_t address) {
    try {
        auto addr = tlb.look_up(address, runningProc->pid);
        return addr;
    } catch (const exception& e) {
        auto pte = runningProc->pageTable.translate(address >> 12);
        auto tlbEntry = tlb.create_tlb_entry(pte.pfn, pte.page_size, address, runningProc->pid);
        tlb.l1_insert(tlbEntry);
        tlb.l2_insert(tlbEntry);
        auto addr = tlb.look_up(address, runningProc->pid);
        return addr;
    }
}

void os::switchToProcess(uint32_t pid) {
    auto it = find_if(processes.begin(), processes.end(), [pid](const process& proc) {
        return proc.pid == pid;
    });

    if (it != processes.end()) {
        // Process found, switch to it
        runningProc = &(*it);
    } else {
        // Process not found, create a new one
        createProcess(pid);
        runningProc = &processes.back();
    }
}

vector<pair<uint32_t, uint32_t> > os::findPhysicalFrames(uint32_t size) {
    size_t pagesNeeded = size / minPageSize;
    size_t freePages = 0;
    size_t start = 0;
    vector<pair<uint32_t, uint32_t> > ret;

    for (size_t i = 0; i < memoryMap.size(); ++i) {
        if (!memoryMap[i]) { // If the page is free
            if (i % (size / minPageSize) != 0) {
                continue;
            }
            if (freePages == 0) start = i;
            freePages++;
            if (freePages == pagesNeeded) {
                for (size_t j = start; j < start + pagesNeeded; ++j) {
                    memoryMap[j] = true;  // Mark pages as allocated
                }
                ret.push_back(make_pair(start, size));
                return ret;
            }
        } else {
            freePages = 0;
        }
    }
    if (size == minPageSize) {
        throw runtime_error("Not enough memory to allocate");
    } else {
        auto temp = findPhysicalFrames(size / 2);
        ret.insert(ret.end(), temp.begin(), temp.end());
        temp = findPhysicalFrames(size / 2);
        ret.insert(ret.end(), temp.begin(), temp.end());
        return ret;
    }
}

