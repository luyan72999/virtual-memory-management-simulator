#include "os.h"
#include "tlb.h"
#include "TwoLevelPageTable.h"
#include <stdint.h>
#include <fstream>
#include <sstream>

int main() {
    size_t memorySize = 1ULL << 32; 
    size_t diskSize = 1024 * 1024 * 1024 * 10; 
    uint32_t high_watermark = 200 * 1024 * 1024;
    uint32_t low_watermark = 100 * 1024 * 1024;

    os osInstance(memorySize, diskSize, high_watermark, low_watermark);

    cout << "OS initialized" << endl;

    ifstream inputFile("tests.txt");
    if (!inputFile) {
        cerr << "Error: Unable to open file." << endl;
        return 1;
    }

    string line;
    while (getline(inputFile, line)) {
        cout << "Executing command: " << line << endl;
        istringstream iss(line);
        uint32_t pid;
        string instruction;
        uint32_t value = 0;

        iss >> pid >> instruction;
        if (instruction == "switch") {
            osInstance.switchToProcess(pid);
        } else {
            if (!(iss >> hex >> value)) {
                cerr << "Error parsing value for instruction: " << instruction << endl;
                continue;
            }
            osInstance.handleInstruction(instruction, value, pid);
        }
    }

    cout << "Total memory access attempts: " << memory_access_attempts << endl;
    cout << "L1 TLB hits: " << L1_hit << endl;
    cout << "L2 TLB hits: " << L2_hit << endl;
    cout << "Memory hits: " << memory_hit << endl;

    inputFile.close();
    return 0;
}