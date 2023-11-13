#include <iostream>
#include <vector>
#include <stdint.h> 
#include <random>
#include <ctime>
#include <cmath>

using namespace std;

class PTEntry {
  public:
    uint32_t page_size;
    uint32_t pfn; 
  
  PTEntry(uint32_t page_size, uint32_t pfn):page_size(page_size),pfn(pfn) {

  }
};

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

//two-level tlb
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
        l2_list->push_back(new vector<TlbEntry>());
      }
      
      l2_process = new vector<uint32_t>();
		}

    // PTEntry get_pt_entry(uint32_t virtual_addr) {
    //   // call page table method to get a page table entry obj

    // }

    // input: a page table entry obj from page table, virtual address
    // output:  a tlb entry obj
    // extract the info from pt obj: process_id, page_size(in bytes?), pfn, calculate vpn based on page size
    TlbEntry create_tlb_entry(PTEntry pt_entry, uint32_t virtual_addr, uint32_t process_id) {
      // calculate mask: number of bits to right shift to extract vpn. 
      // e.g. set mask to 12 when page size is 4KB and physical mem is 4GB (thus 20-bit vpn).
      uint32_t pfn = pt_entry.pfn;
      uint32_t page_size = pt_entry.page_size;
      uint32_t mask = log2(page_size);
      uint32_t vpn = virtual_addr >> mask; 

      TlbEntry tlb_entry = TlbEntry(process_id, page_size, vpn, pfn);
      return tlb_entry;
    }


    // look_up(): given a virtual addr, look it up in both l1 and l2
    // return pfn if found, -1 if miss
    int look_up(uint32_t virtual_addr, uint32_t process_id) {
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
      return -1;
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
      while (l1_list->size() != 0) {
        l1_list->pop_back();
      }
    }
    
    // default: maximum 256 entries allowed per process
    void l2_insert(TlbEntry entry) {
      if (l2_process->size() == 0) {
      // l2 is empty, insert into 0th vector of l2, update l2_process 0th using process_id
        (*l2_list)[0]->push_back(entry);
        l2_process->push_back(entry.process_id);
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
          l2_process->push_back(entry.process_id);
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
      while (sub_vector->size() != 0) {
        sub_vector->pop_back();
      }
    }
    // when a page is swapped out from RAM, delete (invalidate) the corresponding tlb entry
    void l1_remove(uint32_t process_id, uint32_t vpn) {
      for (int i = 0; i < l1_list->size(); i++) {
        if((*l1_list)[i].process_id == process_id) {
          if((*l1_list)[i].vpn == vpn) {
            // remove the tlb entry
            l1_list->erase(l1_list->begin() + i);
            return;
          }
        } else {
          // process not match
          return;
        }
      }
    }

    void l2_remove(uint32_t process_id, uint32_t vpn) {
     // check if process is in l2 
     for(int i = 0; i < l2_process->size(); i++) {
      if((*l2_process)[i] == process_id) {
        //process in l2, check if vpn is in sub-l2
        vector<TlbEntry>* sub_process = (*l2_list)[i];
        for (int j = 0; j < sub_process->size(); j++) {
          if((*sub_process)[j].vpn == vpn) {
            sub_process->erase(sub_process->begin() + j);
            return;
          }
        }
      }
     }
     // otherwise, process not in l2
      return;
    }

    int random_generator(uint32_t start, uint32_t end) {
      mt19937 rng(static_cast<unsigned>(time(nullptr)));
      uniform_int_distribution<int> distribution(start, end);
      int random = distribution(rng);
      return random;
    }

    // destructor
    ~Tlb() {
      delete l1_list;
      for (int i=0; i<max_process_allowed; i++) {
        delete (*l2_list)[i];
      }
      delete l2_list;
      delete l2_process;
    }
};


int main() {

    cout << "test Tlb entry constructor" << endl;
    // params: pid, pz, vpn, pfn
    TlbEntry tlb_entry = TlbEntry(1, 4096, 30, 60);
    cout << tlb_entry.page_size << endl;

    cout << "test Tlb constructor" << endl;
    // params: l1 size, l2 size, max allowed
    Tlb* tlb = new Tlb(64, 1024, 4);
    uint32_t max = tlb->max_process_allowed;
    cout << max << endl;

    // test create_tlb_entry(PTEntry pt_entry, uint32_t virtual_addr, uint32_t process_id)
    cout << "test create_tlb_entry()" << endl;
    PTEntry pt = PTEntry(4096, 100); // pagesize, pfn
    TlbEntry entry_1 = tlb->create_tlb_entry(pt, 1000, 1); 
    cout << entry_1.vpn << endl;
    TlbEntry entry_2 = tlb->create_tlb_entry(pt, 100000, 1);  // expected: 11000 (24) 
    cout << entry_2.vpn << endl;
    TlbEntry entry_3 = tlb->create_tlb_entry(pt, 0, 1); 
    cout << entry_3.vpn << endl;

    //test random generator
    cout << "test random generator" << endl;
    int ran = tlb->random_generator(0,5);
    cout << ran << endl;

    // test l1 insert: Problem: with random generator, everytime the same random is generated
    cout << "test l1 insert" << endl;
    for (int i=0; i<64; i++) {
      tlb->l1_insert(tlb_entry);
    }
    TlbEntry tlb_entry_2 = TlbEntry(2, 8192, 30, 60);
    TlbEntry tlb_entry_3 = TlbEntry(3, 12276, 30, 60);
    tlb->l1_insert(tlb_entry_2);
    tlb->l1_insert(tlb_entry_3);
    for (int i=0; i<64; i++) {
      cout << (*tlb->l1_list)[i].page_size << endl; 
    }
    cout << tlb->l1_list->size() << endl;

    // test l1 flush
    cout << "test l1 flush" << endl;
    tlb->l1_flush();
    cout << tlb->l1_list->size() << endl;
    

    // test l2_insert(TlbEntry entry)
    cout << "test l2 insert" << endl;
    // single process
    cout << "test l2 insert: single process" << endl;
    vector<vector <TlbEntry>*>* temp = tlb->l2_list;
    tlb->l2_insert(tlb_entry);
    cout << (*temp)[0]->size() << endl;
    cout << (*tlb->l2_process)[0] << endl;
    for (int i=0; i<255; i++) {
      tlb->l2_insert(tlb_entry);
    }
    cout << (*temp)[0]->size() << endl; //expected 256
    cout << tlb->l2_process->size() << endl; //expected 1
    
    TlbEntry tlb_entry_4 = TlbEntry(1, 12276, 30, 60);
    tlb->l2_insert(tlb_entry_4);
    for (int i=0; i<256; i++) {
      cout << (*(*temp)[0])[i].page_size << endl;
    }
    cout << (*temp)[0]->size() << endl;  // expected 256 

    // multi processes
    cout << "test l2 insert: multi processes" << endl;
    tlb->l2_insert(tlb_entry_2); // pid 2
    tlb->l2_insert(tlb_entry_3); // pid 3
    TlbEntry tlb_entry_5 = TlbEntry(4, 4096, 30, 60);
    tlb->l2_insert(tlb_entry_5); // pid 4

    cout << (*temp)[1]->size() << endl;   // 1
    cout << (*temp)[2]->size() << endl;   // 1
    cout << (*temp)[3]->size() << endl;   // 1
    cout << tlb->l2_process->size() << endl; //expected 4
    for (int i=0; i<4; i++) {
      cout << (*tlb->l2_process)[i] << endl;  // expected 1 2 3 4
    }

    // insert one more process
    cout << "insert one more process" << endl;
    TlbEntry tlb_entry_6 = TlbEntry(5, 4096, 30, 60);
    tlb->l2_insert(tlb_entry_6); // pid 5
    cout << tlb->l2_process->size() << endl; //expected 4
    for (int i=0; i<4; i++) {
      cout << (*tlb->l2_process)[i] << endl;  // expected not 1 2 3 4
    }

    //test when l2[i] is full
    cout << "test when l2[i] is full" << endl;
    for (int i=0; i<255; i++) {
      tlb->l2_insert(tlb_entry_6);  //pid 5
    }
    for (int i=0; i<4; i++) {
      cout << (*tlb->l2_process)[i] << endl;  // expected not 1 2 3 4
    }
    
    TlbEntry tlb_entry_7 = TlbEntry(5, 16384, 30, 60);
    tlb->l2_insert(tlb_entry_7);
    
    for (int i=0; i<4; i++) {
      if((*tlb->l2_process)[i] == 5) {
        for (int j=0; j<256; j++) {
          uint32_t e = (*(*temp)[i])[j].page_size;
          cout << e << endl;
        }
        cout << (*temp)[i]->size() << endl; //256
      }
    }

    // test look up(va, pid)
    cout << "test look up" << endl;
    int result = tlb->look_up(1, 10);
    cout << result << endl; //-1

    // free
    delete tlb;

    // test remove
    // params: l1 size, l2 size, max allowed
    Tlb* tlb_2 = new Tlb(64, 1024, 4);
    // params: pid, pz, vpn, pfn
    TlbEntry a = TlbEntry(1, 4096, 30, 60);
    TlbEntry b = TlbEntry(1, 4096, 31, 61);
    TlbEntry c = TlbEntry(1, 4096, 32, 62);
    TlbEntry d = TlbEntry(1, 4096, 33, 63);
    TlbEntry e = TlbEntry(1, 4096, 34, 64);

    tlb->l1_insert(a);
    tlb->l1_insert(b);
    tlb->l1_insert(c);
    tlb->l1_insert(d);
    tlb->l1_insert(e);
    tlb->l2_insert(a);
    tlb->l2_insert(b);
    tlb->l2_insert(c);
    tlb->l2_insert(d);
    tlb->l2_insert(e);

    tlb->l1_remove(1, 33);
    cout << tlb->l1_list->size() << endl;  // expected 4

    TlbEntry aa = TlbEntry(2, 4096, 30, 60);
    TlbEntry bb = TlbEntry(2, 4096, 31, 61);
    TlbEntry cc = TlbEntry(2, 4096, 32, 62);
    TlbEntry dd = TlbEntry(2, 4096, 33, 63);
    TlbEntry ee = TlbEntry(2, 4096, 34, 64);

    tlb->l2_insert(aa);
    tlb->l2_insert(bb);
    tlb->l2_insert(cc);
    tlb->l2_insert(dd);
    tlb->l2_insert(ee);

    tlb->l2_remove(1, 33);
    tlb->l2_remove(2, 33);

    cout << (*tlb->l2_list)[0]->size() << endl;  // expected 4
    cout << (*tlb->l2_list)[1]->size() << endl;  // expected 4

    delete tlb_2;
    return 0;
}