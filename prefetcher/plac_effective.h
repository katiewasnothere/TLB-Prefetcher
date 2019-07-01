#ifndef __PLAC_H__
#define __PLAC_H__

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <map>
#include "champsim.h"

typedef struct address_freq {
    uint64_t address;
    uint64_t times_seen;

    address_freq(uint64_t d) : address(d), times_seen(0){}
}addr_freq;


struct Comparator {
    bool const operator() (addr_freq* &a, addr_freq* &b) {
        return (a->times_seen > b->times_seen);
    }
};

class correlation_state {
    private:
        std::map<uint64_t, std::vector<addr_freq*> > corr_map;
        uint64_t S;
        uint64_t lookahead;
        
        uint64_t last_accessed_page;
        uint64_t current_page;
	
    	uint64_t total_triggers;   
	 
        uint64_t get_page_addr(uint64_t full_addr) {
            uint64_t page = (full_addr >> LOG2_PAGE_SIZE) << LOG2_PAGE_SIZE;
            return page;
        }

        void set_current_values(uint64_t full_addr) {
            uint64_t page = get_page_addr(full_addr);
            last_accessed_page = current_page;
            current_page = page;
        }

        // Helper method to see if the address already exists in the freq table
        int find_freq_in_prev() {
            int size = corr_map[last_accessed_page].size();
            for (int i = 0; i < size; i++) {
                if (corr_map[last_accessed_page][i]->address == current_page) {
                    return i;
                }
            }
            return -1;
        }

        void update_count(int index) {
			if (index < 0 || index >= corr_map[last_accessed_page].size()) {
				// an error has occured somehow
				return;
			} 
			addr_freq* addr = corr_map[last_accessed_page][index];
			if (addr->times_seen == UINT_MAX) return;
				
			addr->times_seen += 1;
		}


        void update_prev_correlation() {
            if (current_page == last_accessed_page) {
                return; // if the page address matches, ignore
            }
            uint64_t size = corr_map[last_accessed_page].size();
            int exists_index = find_freq_in_prev();

            if (exists_index != -1) {
                // distance already present in previous predicted, just update
                update_count(exists_index);
            } else {
                // array is not full, we can just push
                addr_freq *new_freq = new  addr_freq(current_page);
                new_freq->times_seen += 1;
                corr_map[last_accessed_page].push_back(new_freq);
            } 
        }

		std::unordered_set<uint64_t> find_predicted_freqs(uint32_t cpu) {
			std::sort(corr_map[current_page].begin(), corr_map[current_page].end(), Comparator());
			
			std::vector<addr_freq*> initial = corr_map[current_page];
			int size = initial.size();
			std::unordered_set<uint64_t> predicted;
			
			int added = 0;
			int index = 0;

			while (added < S && index < size) {
				if (stlb_bloom[cpu]->lookup(initial[index]->address) == 0) {
					predicted.insert(initial[index]->address);
					added += 1;
				}
				index += 1; 
			}
			return predicted;
		}

    public:
        correlation_state(uint64_t slots, uint64_t la) : S(slots),
            lookahead(la), 
            last_accessed_page(0),
            current_page(0),
			total_triggers(0) {}

        ~correlation_state(){}


        std::unordered_set<uint64_t> find_prefetch_addrs(uint64_t full_addr, uint32_t cpu) {
            set_current_values(full_addr);
           
            if (current_page == last_accessed_page) {
                return std::unordered_set<uint64_t>();
            } 
			total_triggers += 1;
            std::unordered_set<uint64_t> result_addrs = find_predicted_freqs(cpu); 

            update_prev_correlation();
            
            // turn a vector of structs into a vector of addresses
            return result_addrs;
        }
		
		uint64_t get_total_triggers() {
			return total_triggers;
		}

};

/* PC-Localized Address Correlation */
class PLAC {
    private:
        uint64_t S;
        uint64_t lookahead;
        std::map<uint64_t, correlation_state*> pc_corr_map;

    public:
        PLAC(uint64_t slots, uint64_t la) : S(slots),
            lookahead(la) {}

        ~PLAC(){}

        std::unordered_set<uint64_t> find_prefetch_addrs(uint64_t addr, uint64_t ip, uint32_t cpu) {
            if (ip == 0) {
                // dummy ip
                return {};
            }
            if (pc_corr_map.count(ip) == 0) {
                correlation_state* new_state = new correlation_state(S, lookahead);
                pc_corr_map[ip] = new_state;
            }   
            return pc_corr_map[ip]->find_prefetch_addrs(addr, cpu);
        }
	
		uint64_t get_total_triggers() {
			uint64_t result = 0;
			for (auto pair: pc_corr_map) {
				result += pair.second->get_total_triggers();
			}
			return result;
		}
        
};

#endif // __PLAC_H__
