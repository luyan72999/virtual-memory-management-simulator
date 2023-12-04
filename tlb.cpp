#include <iostream>
#include <algorithm>
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
  
  srand(time(NULL));
  cout << "TLB initialized" << endl;
}

// destructor
Tlb::~Tlb() {
  delete l1_list;
  for (int i=0; i<max_process_allowed; i++) {
    delete (*l2_list)[i];
  }
  delete l2_list;
}


// pfn and page_size is obtained from page table entry obj
TlbEntry Tlb::create_tlb_entry(uint32_t pfn, uint32_t page_size, uint32_t virtual_addr, uint32_t process_id) {
  // calculate mask: number of bits to right shift to extract vpn.
  // e.g. set mask to 12 when page size is 4KB and physical mem is 4GB (thus 20-bit vpn).
  uint32_t mask = ~(page_size - 1);
  uint32_t vpn = (virtual_addr & mask) >> 12;

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
    uint32_t vpn = (virtual_addr & mask) >> 12;
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
      uint32_t vpn = (virtual_addr & mask) >> 12;
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

// TLBs: insert a tlb entry into l1 using FIFO policy
// return -1 if no replacement occurs, return the replaced index in l1 if replacement occurs.
int Tlb::l1_insert(TlbEntry entry, int fifo) {
  if(l1_list->size() < l1_size) {
    l1_list->push_back(entry);
    return -1;
  } else {
    // l1 is full, kick the first element out
    l1_list->erase(l1_list->begin());
    l1_list->push_back(entry);
    return 0;  // return the index of replaced element
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
    // std::find_if() is used for searching a range defined by iterators for the first element that satisfies a specific condition.
    // check if that process is already in tlb l2 list
    auto iter = find_if(l2_list->begin(), l2_list->end(), [entry](const vector<TlbEntry>* procTlb) {
        return !procTlb->empty() && procTlb->back().process_id == entry.process_id;
    });
    vector<TlbEntry>* toReplace = nullptr;
    if (iter == l2_list->end()) {
        // the process is not in l2 tlb
        // find the first empty proTlb (sub l2 tlb)
        auto iter = find_if(l2_list->begin(), l2_list->end(), [](const vector<TlbEntry>* procTlb) {
            return procTlb->empty();
        });
        if (iter == l2_list->end()) {
            // no empty sub list is found, reached max_process_allowed
            // pick a random sub list and flush all the enties of that sub list
            auto idx = random_generator(0, max_process_allowed);
            toReplace = (*l2_list)[idx];
            toReplace->clear();
        } else {
            // an empty sub list is found
            toReplace = *iter;
        }
    } else {
      // the process is already in l2 tlb
        toReplace = *iter;
    }
    if (toReplace->size() == l2_size_per_process) {
      // the sub l2 is already full, pick a random one to replace
        int idx = random_generator(0, l2_size_per_process);
        (*toReplace)[idx] = entry;
    } else {
      // sub l2 is not null, push back
        toReplace->push_back(entry);
    }
}

void Tlb::l2_insert(TlbEntry entry, int fifo) {
    // std::find_if() is used for searching a range defined by iterators for the first element that satisfies a specific condition.
    // check if that process is already in tlb l2 list
    auto iter = find_if(l2_list->begin(), l2_list->end(), [entry](const vector<TlbEntry>* procTlb) {
        return !procTlb->empty() && procTlb->back().process_id == entry.process_id;
    });
    vector<TlbEntry>* toReplace = nullptr;
    if (iter == l2_list->end()) {
        // the process is not in l2 tlb
        // find the first empty proTlb (sub l2 tlb)
        auto iter = find_if(l2_list->begin(), l2_list->end(), [](const vector<TlbEntry>* procTlb) {
            return procTlb->empty();
        });
        if (iter == l2_list->end()) {
            // no empty sub list is found, reached max_process_allowed
            // delete the first sub list and insert a new empty sub list at the end
            delete l2_list->front();
            l2_list->erase(l2_list->begin());
            l2_list->push_back(new vector<TlbEntry>());
            toReplace = l2_list->back();
        } else {
            // an empty sub list is found
            toReplace = *iter;
        }
    } else {
      // the process is already in l2 tlb
        toReplace = *iter;
    }
    if (toReplace->size() == l2_size_per_process) {
      // the sub l2 is already full, pick the first one to remove, and insert at the end (FIFO)
        toReplace->erase(toReplace->begin());
        toReplace->push_back(entry);
    } else {
      // sub l2 is not null, push back
        toReplace->push_back(entry);
    }
}

// int Tlb::replacingPolicy(int size) {
//     return random_generator(0, l2_size_per_process);
// }


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

// when a page is swapped out from RAM, delete (invalidate) the corresponding tlb entry
void Tlb::l2_remove(uint32_t process_id, uint32_t vpn) {
  // check if process is in l2 
  for (int i = 0; i < l2_list->size(); i++) {
    vector<TlbEntry>* sub_process = (*l2_list)[i];
    if (!sub_process->empty()) {
      if (sub_process->back().process_id == process_id) {
        for (int j = 0; j < sub_process->size(); j++) {
          if ((*sub_process)[j].vpn == vpn) {
            sub_process->erase(sub_process->begin() + j);
            return;
          }
        }
      }
    }
  }
  // otherwise, process not in l2 or process_id not found
  return;
}

int Tlb::random_generator(uint32_t start, uint32_t end) {
  int span = end - start;
  int random = rand() % span + start;
  return random;
}

PTEntry::PTEntry(uint32_t page_size, uint32_t pfn):page_size(page_size),pfn(pfn) {}


// test FIFO
// test tlb look up return value
int main() {

    // test FIFO insert and invalidate_tlb
    // params: l1 size, l2 size, max allowed
    Tlb* tlb = new Tlb(64, 1024, 4);
    // params: pid, pz, vpn, pfn

    TlbEntry a = TlbEntry(1, 4096, 30, 60);
    TlbEntry b = TlbEntry(1, 4096, 31, 61);
    TlbEntry c = TlbEntry(1, 4096, 32, 62);
    cout << "test l1 FIFO insert" << endl;
    tlb->l1_insert(a,0); // a will disappear
    for (int i=0; i<63; i++) {
      tlb->l1_insert(b,0);
    }
    tlb->l1_insert(c,0);  // c will be the end

    for (int i=0; i<tlb->l1_list->size(); i++) {
      vector <TlbEntry>* temp = tlb->l1_list;
      cout << (*temp)[i].vpn << endl;
    }


    cout << "test l2 FIFO insert" << endl;
    tlb->l2_insert(a,0);
    for (int i=0; i<255; i++) {
      tlb->l2_insert(b,0);  //pid 5
    }
    tlb->l2_insert(c,0);

    vector <vector <TlbEntry>*>* l2 = tlb->l2_list;
    vector <TlbEntry>* temp = (*l2)[0];
    cout << temp->front().vpn << endl; // expected 31
    cout << temp->back().vpn << endl;  // expected 32

    // TlbEntry a = TlbEntry(1, 4096, 30, 60);
    // TlbEntry b = TlbEntry(1, 4096, 31, 61);
    // TlbEntry c = TlbEntry(1, 4096, 32, 62);
    // TlbEntry d = TlbEntry(1, 4096, 33, 63);
    // TlbEntry e = TlbEntry(1, 4096, 34, 64);

    // tlb->l1_insert(a,0);
    // tlb->l1_insert(b,0);
    // tlb->l1_insert(c,0);
    // tlb->l1_insert(d,0);
    // tlb->l1_insert(e,0);
    // tlb->l2_insert(a,0);
    // tlb->l2_insert(b,0);
    // tlb->l2_insert(c,0);
    // tlb->l2_insert(d,0);
    // tlb->l2_insert(e,0);

    // TlbEntry aa = TlbEntry(2, 4096, 30, 60);
    // TlbEntry bb = TlbEntry(2, 4096, 31, 61);
    // TlbEntry cc = TlbEntry(2, 4096, 32, 62);
    // TlbEntry dd = TlbEntry(2, 4096, 33, 63);
    // TlbEntry ee = TlbEntry(2, 4096, 34, 64);

    // tlb->l2_insert(aa,0);
    // tlb->l2_insert(bb,0);
    // tlb->l2_insert(cc,0);
    // tlb->l2_insert(dd,0);
    // tlb->l2_insert(ee,0);

    // tlb->invalidate_tlb(1, 33); //process_id, vpn
    
    // cout << tlb->l1_list->size() << endl;  // expected 4
    // cout << (*tlb->l2_list)[0]->size() << endl;  // expected 4
    // cout << (*tlb->l2_list)[1]->size() << endl;  // expected 5

    // tlb->invalidate_tlb(2, 34);
    // cout << (*tlb->l2_list)[1]->size() << endl;  // expected 4

    delete tlb;

    

    cout << "No error occurs" << endl;
    return 0;
}