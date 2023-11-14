#include <iostream>
#include "tlb.h"

int L1_hit = 0;
int L2_hit = 0;
int TLB_miss = 0;

// constructor
TlbEntry::TlbEntry(uint32_t process_id, uint32_t page_size, uint32_t vpn, uint32_t pfn) : process_id(process_id),page_size(page_size),vpn(vpn), pfn(pfn), reference(1) {}


//two-level tlb
// constructor
Tlb::Tlb(uint32_t l1_size, uint32_t l2_size, uint32_t max_process_allowed) : l1_size(l1_size), l2_size(l2_size), max_process_allowed(max_process_allowed) {
  // by default: l1 size 64, l2 size 1024, max process allowed is 4
  l1_list = new vector<TlbEntry>();
  l2_list = new vector<vector<TlbEntry>*>();

  l2_size_per_process = l2_size / max_process_allowed;
  for (int i = 0; i < max_process_allowed; i++) {
    l2_list->push_back(new vector<TlbEntry>());
  }
  
  l2_process = new vector<uint32_t>();
  srand(time(NULL));
  cout << "TLB initialized" << endl;
}

// Tlb::Tlb(uint32_t l1_size, uint32_t l2_size, uint32_t max_process_allowed) {
//   l1_size = l1_size;
//   l2_size = l2_size;
//   max_process_allowed = max_process_allowed;
// }

// destructor
Tlb::~Tlb() {
  delete l1_list;
  for (int i=0; i<max_process_allowed; i++) {
    delete (*l2_list)[i];
  }
  delete l2_list;
  delete l2_process;
}


// pfn and page_size is obtained from page table entry obj
TlbEntry Tlb::create_tlb_entry(uint32_t pfn, uint32_t page_size, uint32_t virtual_addr, uint32_t process_id) {
  // calculate mask: number of bits to right shift to extract vpn.
  // e.g. set mask to 12 when page size is 4KB and physical mem is 4GB (thus 20-bit vpn).
  uint32_t mask = ~(page_size - 1);
  uint32_t vpn = virtual_addr & mask >> 12;

  TlbEntry tlb_entry = TlbEntry(process_id, page_size, vpn, pfn);
    return tlb_entry;
}


// look_up(): given a virtual addr, look it up in both l1 and l2
// return pfn if found, -1 if miss
int Tlb::look_up(uint32_t virtual_addr, uint32_t process_id) {
  // first, check l1
  // loop through l1, compute the virtual addr vpn using tlb entry page size, and compare the 2 vpns
  for (int i = 0; i < l1_list->size(); i++) {
    uint32_t page_size = (*l1_list)[i].page_size;
    uint32_t mask = ~(page_size - 1);
    uint32_t vpn = virtual_addr & mask >> 12;
    if (vpn == (*l1_list)[i].vpn) {
      L1_hit++;
      return (*l1_list)[i].pfn;
    }
  }

  // If only 1 level TLB is supported, uncomment this
  /*
  TLB_miss++;
  throw logic_error("TLB miss");
   */

  // not in l1, check l2:
  // first, check if the process is already in l2
  for (int i = 0; i < l2_list->size(); i++) {
    if ((*l2_list)[i]->empty())
      continue;
    if ((*l2_list)[i]->back().process_id != process_id)
      continue;
    for (int j = 0; j < (*l2_list)[i]->size(); j++) {
      TlbEntry entry = (*((*l2_list)[i]))[j];
      uint32_t page_size = entry.page_size;
      uint32_t mask = ~(page_size - 1);
      uint32_t vpn = virtual_addr & mask >> 12;
      if (vpn == entry.vpn) {
        // found in l2, insert this one into l1
        l1_insert(entry);
        L2_hit++;
        return entry.pfn;
      }
    }
  }
  // otherwise, l2 miss, go to page table with virtual addr and get a page table entry
  TLB_miss++;
  throw logic_error("TLB miss");
}


// upon TLB hit, assemble physical address: use pfn and offset to form a physicai address
uint32_t Tlb::assemble_physical_addr(TlbEntry tlb_entry, uint32_t virtual_addr) {
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
int Tlb::l1_insert(TlbEntry entry) {
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
void Tlb::l1_flush() {
  while (l1_list->size() != 0) {
    l1_list->pop_back();
  }
}

// default: maximum 256 entries allowed per process
void Tlb::l2_insert(TlbEntry entry) {
    auto iter = std::find_if(l2_list->begin(), l2_list->end(), [entry](const vector<TlbEntry>* procTlb) {
        return !procTlb->empty() && procTlb->back().process_id == entry.process_id;
    });
    vector<TlbEntry>* toReplace = nullptr;
    if (iter == l2_list->end()) {
        // current process not in l2 tlb
        auto iter = std::find_if(l2_list->begin(), l2_list->end(), [](const vector<TlbEntry>* procTlb) {
            return procTlb->empty();
        });
        if (iter == l2_list->end()) {
            // reached max_process_allowed
            auto idx = random_generator(0, max_process_allowed);
            toReplace = (*l2_list)[idx];
            toReplace->clear();
        } else {
            toReplace = *iter;
        }
    } else {
        toReplace = *iter;
    }
    if (toReplace->size() == l2_size_per_process) {
        int idx = replacingPolicy(l2_size_per_process);
        (*toReplace)[idx] = entry;
    } else {
        toReplace->push_back(entry);
    }
}

int Tlb::replacingPolicy(int size) {
    return random_generator(0, l2_size_per_process);
}

// selective flushing
void Tlb::l2_flush(vector<TlbEntry>* sub_vector) {
  while (sub_vector->size() != 0) {
    sub_vector->pop_back();
  }
}

void Tlb::invalidate_tlb(uint32_t process_id, uint32_t vpn) {
  l1_remove(process_id, vpn);
  l2_remove(process_id, vpn);
  return;
}

// when a page is swapped out from RAM, delete (invalidate) the corresponding tlb entry
void Tlb::l1_remove(uint32_t process_id, uint32_t vpn) {
  for (int i = 0; i < l1_list->size(); i++) {
    if ( (*l1_list)[i].process_id == process_id ) {
      if ( (*l1_list)[i].vpn == vpn ) {
        // remove the tlb entry
        l1_list->erase(l1_list->begin() + i);
        return;
      }
    }
    else {
      // process not match
      return;
    }
  }
}

void Tlb::l2_remove(uint32_t process_id, uint32_t vpn) {
  // check if process is in l2 
  for (int i = 0; i < l2_process->size(); i++) {
    if ( (*l2_process)[i] == process_id) {
      //process in l2, check if vpn is in sub-l2
      vector<TlbEntry>* sub_process = (*l2_list)[i];
      for (int j = 0; j < sub_process->size(); j++) {
        if( (*sub_process)[j].vpn == vpn ) {
          sub_process->erase(sub_process->begin() + j);
          return;
        }
      }
    }
  }

  // otherwise, process not in l2
  return;
}

int Tlb::random_generator(uint32_t start, uint32_t end) {
  int span = end - start;
  int random = rand() % span + start;
  return random;
}