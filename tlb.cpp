// 还没写完，结构也会再调整
#include <iostream>
#include <vector>
#include <stdint.h> 
#include <random>
#include <ctime>
#include <cmath>

using namespace std;

// the entry in TLBs
class TlbEntry {
  public:
    uint32_t process_id; // get it from page table entry obj
    uint32_t page_size;  // different page has different sizes, get it from page table entry obj
                         // set mask according to page_size
    uint32_t reference;  // only needed in LRU replacement policy

    uint32_t vpn; 
    uint32_t pfn; 
    
    // constructor
    TlbEntry(uint32_t process_id, uint32_t page_size, uint32_t vpn, uint32_t pfn):process_id(process_id),page_size(page_size),vpn(vpn), pfn(pfn){
				reference = 1;
		}
};

// two-level tlb
class Tlb {
  public:
    vector<TlbEntry*>* l1_list;
    vector<vector<TlbEntry*>*>* l2_list;   
    uint32_t l1_size;
    uint32_t l2_size;
    uint32_t max_process_allowed; // max number of processes that can exist in l2, default 4
    uint32_t l2_size_per_process; // default 1024/4 = 256

    // constructor
		Tlb(uint32_t l1_size, uint32_t l2_size, uint32_t max_process_allowed):l1_size(l1_size),l2_size(l2_size), max_process_allowed(max_process_allowed){
			//by default: l1 size 64, l2 size 1024, max process allowed is 4

      l1_list = new vector<TlbEntry*>();
      l2_list = new vector<vector<TlbEntry*>*>();

      for (int i = 0; i < max_process_allowed; i++) {
        (*l2_list)[i] = new vector<TlbEntry*>();
      }

      l2_size_per_process = l2_size / max_process_allowed;
		}

    PTEntry* get_pt_entry(uint32_t virtual_addr) {
      // call page table method to get a page table entry obj

    }

    // input: a page table entry obj from page table, virtual address
    // output:  a tlb entry obj
    // extract the info from pt obj: process_id, page_size(in bytes?), pfn, calculate vpn based on page size
    TlbEntry* create_tlb_entry(PTEntry* pt_entry, uint32_t virtual_addr) {
      // calculate mask: number of bits to right shift to extract vpn. 
      // e.g. set mask to 12 when page size is 4KB and physical mem is 4GB (thus 20-bit vpn).
      uint32_t page_size = pt_entry->page_size;
      uint32_t mask = log2(page_size);
      uint32_t vpn = virtual_addr >> mask; 

      uint32_t pfn = pt_entry->pfn;
      uint32_t process_id = pt_entry->process_id;

      TlbEntry* tlb_entry = new TlbEntry(process_id, page_size, vpn, pfn);
      return tlb_entry;
    }


    // look_up(): given a vpn, look it up in both l1 and l2
    // returns pfn if found
    // returns -1 if not found, os needs to handle this exception
    uint32_t look_up(uint32_t virtual_addr, uint32_t page_size, uint32_t process_id) {
      // exact vpn based on page_size
      uint32_t mask = log2(page_size);
      uint32_t vpn = virtual_addr >> mask;

      // first, check l1
      for (int i = 0; i < l1_list->size(); i++) {
        if ((*l1_list)[i]->vpn == vpn) {
          return (*l1_list)[i]->pfn;
        }
      }
      // not in l1, check l2: TBD
      // for (int i = 0; i < l2_list->size(); i++) {
      //   if ((*l2_list)[i]->vpn == vpn) {
      //     // if found, insert this item to l1
      //     l1_insert((*l2_list)[i]);
      //     return (*l2_list)[i]->pfn;
      //   }
      // }
      // how to return if not found? can return -1 but return type is unsigned int 
      // not found in both tlbs: os goes to page table and get pfn and meta data
      
    }


    // upon TLB hit, assemble physical address: if found in tlb, use pfn and offset to form a physicai address
    uint32_t assemble_physical_addr(TlbEntry* tlb_entry, uint32_t virtual_addr) {
      // get the offset length based on page size
      uint32_t page_size = tlb_entry->page_size;
      uint32_t offset_length = log2(page_size);

      // get the value of offset
      uint32_t mask = (1 << offset_length) - 1;
      uint32_t offset = virtual_addr & mask;

      // assembly pfn + offset
      uint32_t physical_addr; // TBD
      return physical_addr;
    }


    // TLBs: insert a tlb entry into l1
    void l1_insert(TlbEntry* entry) {
      if(l1_list->size() < l1_size) {
        l1_list->push_back(entry);
      } else {
        // l1 is full, pick a random one to replace
        int random = random_generator(0, l1_size);
        (*l1_list)[random] = entry;
      }
      return;
    }

    //flush all
    void l1_flush() {
      for (int i = 0; i < l1_list->size(); i++) {
        l1_list->pop_back();
      }
    }
    
    // default: maximum 256 entries allowed per process
    void l2_insert(TlbEntry* entry) {
      
    }

    // selective flushing
    void l2_flush(TlbEntry* entry) {
      
    }

    int random_generator(int start, int end) {
      mt19937 rng(static_cast<unsigned>(time(nullptr)));
      uniform_int_distribution<int> distribution(start, end);
      int random = distribution(rng);
      return random;
    }

};


int main() {
   
    return 0;
}