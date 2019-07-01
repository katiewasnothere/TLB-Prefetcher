#ifndef __GAC_H__
#define __GAC_H__

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <map>
#include "champsim.h"

typedef struct address_freq {
	uint64_t address;
	uint64_t times_seen;;

	address_freq(uint64_t d) : address(d), times_seen(0){}
}addr_freq;

struct Comparator {

	bool const operator() (addr_freq* &a, addr_freq* &b) const{
		return (a->times_seen > b->times_seen);
	}
};

class GAC {
	private:
		// TODO the predicted values are supposed to be managed with LRU
		uint64_t S;
		uint64_t lookahead;
		std::map<uint64_t, std::vector<addr_freq*>> corr_map;

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
			} else  {
				// array is not full, we can just push
				addr_freq *new_freq = new  addr_freq(current_page);
				new_freq->times_seen = 1;
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
			//printf("initial size is %ld\n", size);
            while (added < S && index < size) {
				// TODO: get cpu
				// printf("Result from bloom filter is %ld\n", stlb_bloom[cpu]->lookup(initial[index]->address)); 
                if (stlb_bloom[cpu]->lookup(initial[index]->address) == 0) {
				// if (stlb_set[cpu].count(initial[index]->address) == 0) {
					// this address is not present in the tlb, predict it
					// printf("addind to predicted\n");
					predicted.insert(initial[index]->address);
					added += 1;
				}
				index += 1;
            }
			// printf("added %ld\n", added);
			return predicted;
		}

	public:
		GAC(uint64_t slots, uint64_t lookahead_amount) : S(slots),
			lookahead(lookahead_amount),
			last_accessed_page(0),
			current_page(0),
			total_triggers(0) {}

		~GAC(){}


		std::unordered_set<uint64_t> find_prefetch_addrs(uint64_t full_addr, uint32_t cpu) {
			set_current_values(full_addr);

			if (current_page == last_accessed_page) return {};
			total_triggers += 1;
			std::unordered_set<uint64_t> predicted = find_predicted_freqs(cpu);
			update_prev_correlation();
			

			// turn a vector of structs into a vector of addresses
			return predicted;
		}
		
		uint64_t get_total_triggers() {
            return total_triggers;
        }

};

#endif // __GAC_H__
