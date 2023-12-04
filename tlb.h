// tlb.h
#ifndef TLB_H 
#define TLB_H

#include <stdint.h>
#include <vector>
#include <random>
#include <ctime>
#include <cmath>

using namespace std;

extern int L1_hit;
extern int L2_hit;
extern int TLB_miss;

class TlbEntry {
public:
  uint32_t process_id; // get it from page table entry obj
  uint32_t page_size;  // different page has different sizes, get it from page table entry obj
                       // set mask according to page_size
  uint32_t vpn;
  uint32_t pfn;
  uint32_t reference;  // only needed in LRU replacement policy

  // constructor
  TlbEntry(uint32_t process_id, uint32_t page_size, uint32_t vpn, uint32_t pfn);
};

class PTEntry {
public:
  uint32_t page_size;
  uint32_t pfn;

  PTEntry(uint32_t page_size, uint32_t pfn);
};

//two-level tlb
class Tlb {
public:
  vector<TlbEntry>* l1_list;
  vector<vector<TlbEntry>*>* l2_list;   
  uint32_t l1_size;
  uint32_t l2_size;
  uint32_t max_process_allowed; // max number of processes that can exist in l2, default 4
  uint32_t l2_size_per_process; // default 1024/4 = 256

  // constructor
	Tlb(uint32_t l1_size, uint32_t l2_size, uint32_t max_process_allowed);

  // destructor
  ~Tlb();

  // pfn and page_size is obtained from page table entry obj
  TlbEntry create_tlb_entry(uint32_t pfn, uint32_t page_size, uint32_t virtual_addr, uint32_t process_id);

  // look_up(): given a virtual addr, look it up in both l1 and l2
  // return pfn if found, -1 if miss
  int look_up(uint32_t virtual_addr, uint32_t process_id);

  // upon TLB hit, assemble physical address: use pfn and offset to form a physicai address
  uint32_t assemble_physical_addr(TlbEntry tlb_entry, uint32_t virtual_addr);

  // TLBs: insert a tlb entry into l1, random policy
  // return -1 if no replacement occurs, return the replaced index in l1 if replacement occurs.
  int l1_insert(TlbEntry entry);
  // the following l1_insert() implememts a fifo policy. When calling the method, the parameter fifo can be any number (it's added only for method overloading)
  int l1_insert(TlbEntry entry, int fifo);
  //flush all
  void l1_flush();
  
  // default: maximum 256 entries allowed per process
  // random policy
  void l2_insert(TlbEntry entry);
  // the following l2_insert() implememts a fifo policy. When calling the method, the parameter fifo can be any number (it's added only for method overloading)
  void l2_insert(TlbEntry entry, int fifo);

  void invalidate_tlb(uint32_t process_id, uint32_t vpn);

private:
  // when a page is swapped out from RAM, delete (invalidate) the corresponding tlb entry
  void l1_remove(uint32_t process_id, uint32_t vpn);

  void l2_remove(uint32_t process_id, uint32_t vpn);

  int random_generator(uint32_t start, uint32_t end);

  //int replacingPolicy(int size);
};

#endif