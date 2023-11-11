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
    vector<TlbEntry>* l1_list;
    vector<vector<TlbEntry>*>* l2_list;   
    uint32_t l1_size;
    uint32_t l2_size;
    uint32_t max_process_allowed; // max number of processes that can exist in l2, default 4
    uint32_t l2_size_per_process; // default 1024/4 = 256
    vector<uint32_t>* l2_process; // extra data structure to track which processes are in L2 

    // constructor
		Tlb(uint32_t l1_size, uint32_t l2_size, uint32_t max_process_allowed):l1_size(l1_size), l2_size(l2_size), max_process_allowed(max_process_allowed){
			//by default: l1 size 64, l2 size 1024, max process allowed is 4

      l1_list = new vector<TlbEntry>();
      l2_list = new vector<vector<TlbEntry>*>();

      l2_size_per_process = l2_size / max_process_allowed;
      for (int i = 0; i < max_process_allowed; i++) {
        (*l2_list)[i] = new vector<TlbEntry>();
      }
      
      l2_process = new vector<uint32_t>();
		}

    PTEntry get_pt_entry(uint32_t virtual_addr) {
      // call page table method to get a page table entry obj

    }

    // input: a page table entry obj from page table, virtual address
    // output:  a tlb entry obj
    // extract the info from pt obj: process_id, page_size(in bytes?), pfn, calculate vpn based on page size
    TlbEntry create_tlb_entry(PTEntry pt_entry, uint32_t virtual_addr) {
      // calculate mask: number of bits to right shift to extract vpn. 
      // e.g. set mask to 12 when page size is 4KB and physical mem is 4GB (thus 20-bit vpn).
      uint32_t page_size = pt_entry.page_size;
      uint32_t mask = log2(page_size);
      uint32_t vpn = virtual_addr >> mask; 

      uint32_t pfn = pt_entry.pfn;
      uint32_t process_id = pt_entry.process_id;

      TlbEntry tlb_entry = TlbEntry(process_id, page_size, vpn, pfn);
      return tlb_entry;
    }


    // look_up(): given a vpn, look it up in both l1 and l2
    // returns pfn if found
    // TBD: how to signal a TLB miss if cannot specify a negative number? return 0 and let pfn starts with 1?
    uint32_t look_up(uint32_t virtual_addr, uint32_t process_id) {
      // first, check l1
      // loop through l1, compute the virtual addr vpn using tlb entry page size, and compare the 2 vpns
      for (int i = 0; i < l1_list->size(); i++) {
        uint32_t page_size = (*l1_list)[i].page_size;
        uint32_t mask = log2(page_size);
        uint32_t vpn = virtual_addr >> mask;
        if (vpn == (*l1_list)[i].vpn) {
          return (*l1_list)[i].pfn;
        }
      }

      // not in l1, check l2: 
      // first, check if the process is already in l2
      for (int i = 0; i < max_process_allowed; i++) {
        if(process_id == (*l2_process)[i]) {
          // process already in l2, check corresponding sub l2 cache
          for (int j = 0; j < l2_size_per_process; j++) {
            vector<TlbEntry>* sub_list = (*l2_list)[i];
            uint32_t page_size_2 = (*sub_list)[j].page_size;
            uint32_t mask_2 = log2(page_size_2);
            uint32_t vpn_2 = virtual_addr >> mask_2;
            if (vpn_2 == (*sub_list)[j].vpn) {
              // found in l2, insert this one into l1
              l1_insert((*sub_list)[j]);
              return (*sub_list)[j].pfn;
            }
          }
        }
      }
      // otherwise, l2 miss, go to page table with virtual addr and get a page table entry
    }


    // upon TLB hit, assemble physical address: use pfn and offset to form a physicai address
    uint32_t assemble_physical_addr(TlbEntry tlb_entry, uint32_t virtual_addr) {
      // get the offset length based on page size
      uint32_t page_size = tlb_entry.page_size;
      uint32_t offset_length = log2(page_size);

      // get the value of offset
      uint32_t mask = (1 << offset_length) - 1;
      uint32_t offset = virtual_addr & mask;

      // assembly pfn + offset
      uint32_t pfn = tlb_entry.pfn;
      uint32_t physical_addr = (pfn << offset_length) | offset;
      return physical_addr;
    }

    // TLBs: insert a tlb entry into l1
    // return -1 if no replacement occurs, return the replaced index in l1 if replacement occurs.
    int l1_insert(TlbEntry entry) {
      if(l1_list->size() < l1_size) {
        l1_list->push_back(entry);
        return -1;
      } else {
        // l1 is full, pick a random one to replace
        int random = random_generator(0, l1_size-1);
        (*l1_list)[random] = entry;
        return random;
      }
    }

    //flush all
    void l1_flush() {
      for (int i = 0; i < l1_list->size(); i++) {
        l1_list->pop_back();
      }
    }
    
    // default: maximum 256 entries allowed per process
    void l2_insert(TlbEntry entry) {
      if (l2_process->size() == 0) {
      // l2 is empty, insert into 0th vector of l2, update l2_process 0th using process_id
        (*l2_list)[0]->push_back(entry);
        (*l2_process)[0] = entry.process_id;
        return;
      } else {
        // check if this process is already in l2
        for(int i = 0; i < l2_process->size(); i++) {
          if((*l2_process)[i] == entry.process_id) {
            // process cache already in l2, update the corresponding process cache vector
            if ((*l2_list)[i]->size() < l2_size_per_process) {
              // l2[i] not full, insert
              (*l2_list)[i]->push_back(entry);
              return;
            } else {
              //l2[i] is full, pick a random one to replace
              int random = random_generator(0, l2_size_per_process-1);
              (*(*l2_list)[i])[random] = entry;
              return;
            }
          }
        }
        // process not in l2
        // check if all 4 sub-l2 is in use
        if (l2_process->size() < max_process_allowed) {
          // not all 4 sub-l2 is in use
          uint32_t index = l2_process->size(); 
          (*l2_process)[index] = entry.process_id;
          (*l2_list)[index]->push_back(entry);
          return;
        } else {
          // all 4 sub-l2 is in use: replace a random one, and insert
          int random = random_generator(0, max_process_allowed-1);
          (*l2_process)[random] = entry.process_id;
          l2_flush((*l2_list)[random]);
          (*l2_list)[random]->push_back(entry);
          return;
        }
      }   
    }

    // selective flushing
    void l2_flush(vector<TlbEntry>* sub_vector) {
      for (int i = 0; i < sub_vector->size(); i++) {
        sub_vector->pop_back();
      }
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