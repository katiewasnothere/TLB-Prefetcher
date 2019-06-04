#ifndef __GAC_L_H__
#define __GAC_L_H__

#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <map>
#include <math.h>
#include "champsim.h"

typedef struct address_freq {
	uint64_t address;
	uint64_t lru;

	address_freq(uint64_t d) : address(d), lru(0){}
	bool operator ==(const struct address_freq* &a) const {
		return this->address == a->address;
	}
}addr_freq;

struct addr_freq_comparator {
	bool operator()(const addr_freq* a, const addr_freq* b) const {
		return a->address == b->address;
	}
};

struct addr_freq_hash {
	bool operator()(const addr_freq* a) const {
		return std::hash<uint64_t>()(a->address);
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

		uint64_t get_page_addr(uint64_t full_addr) {
			uint64_t page = (full_addr >> LOG2_PAGE_SIZE) << LOG2_PAGE_SIZE;
			return page;
		}

		void set_current_values(uint64_t full_addr) {
			uint64_t page = get_page_addr(full_addr);
			current_page = page;
		}

		void update_previous_values() {
			last_accessed_page = current_page;
			current_page = 0;
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

		void lru_update(int index) {
			int max = corr_map[last_accessed_page].size();
			for (int i = 0; i < max; i++) {
				corr_map[last_accessed_page][i]->lru++;
			}
			corr_map[last_accessed_page][index]->lru = 0;
		}

		// only call when lru is full
		int lru_evict() {
			for (uint64_t i = 0; i < S; i++) {
				if (corr_map[last_accessed_page][i]->lru == S - 1) {
					// we've found a value to evict, replace with current
					corr_map[last_accessed_page][i]->address = current_page;
					corr_map[last_accessed_page][i]->lru = 0;
					return i;
				}
			}
			// TODO: return an error code?
			return 0;
		}

		void update_prev_correlation() {
			if (current_page == last_accessed_page) {
				return; // if the page address matches, ignore
			}
			uint64_t size = corr_map[last_accessed_page].size();
			int exists_index = find_freq_in_prev();

			if (exists_index != -1) {
				// distance already present in previous predicted, just update
				lru_update(exists_index);
			} else if (size < S) {
				// array is not full, we can just push
				addr_freq *new_freq = new  addr_freq(current_page);
				corr_map[last_accessed_page].push_back(new_freq);
				lru_update(size);
			} else {
				// array is full, find value to evict
				int index = lru_evict();
				lru_update(index);
			}
		}

		std::vector<uint64_t> freq_to_addresses(std::vector<addr_freq*> &freqs) {
			int size = freqs.size();
			if (size == 0) {
				return {};
			}
			std::vector<uint64_t> addrs;
			for (int i = 0; i < size; i++) {
				addrs.push_back(freqs[i]->address);
			}
			return addrs;
		}

		std::vector<uint64_t> freq_to_addresses(std::unordered_set<uint64_t> &freqs) {
			int size = freqs.size();
			if (size <= 0) {
				return {};
			}
			
			std::vector<uint64_t> addrs;
			for (auto itr = freqs.begin(); itr != freqs.end(); ++itr) {
				addrs.push_back(*itr);
			}		
			return addrs;
		}

		int find_mru_index(std::vector<addr_freq*> correlated) {
			for (int i = 0; i < correlated.size(); i++) {
				if (correlated[i]->lru == 0) {
					return i;
				}
			}
			// should not reach here, consider returning error 
			return -1;
		}		

		/*std::unordered_set<uint64_t> find_predicted_freqs() {
			std::vector<addr_freq*> initial = corr_map[current_page]; 
			
			std::unordered_set<uint64_t> predicted; 
			
			for (int i = 0; i < initial.size(); i++) {
				uint64_t temp_address = initial[i]->address; 
				std::vector<addr_freq*> correlated;
				for (int j = 0; j < lookahead; j++) {
					correlated = corr_map[temp_address];
					if (correlated.size() <= 0) break;
					int mru = find_mru_index(correlated);
					temp_address = correlated[mru]->address;
				}	
				predicted.insert(temp_address);
			}
			return predicted;
		}*/

		/*std::unordered_set<uint64_t> find_predicted_freqs() {
			std::queue<uint64_t> to_visit;
			to_visit.push(current_page);
			
			std::unordered_set<uint64_t> predicted;
			int current_depth = 0;
			while (!to_visit.empty() && current_depth <= lookahead) {
				uint64_t addr = to_visit.top();
				to_visit.pop();
				
				std::vector<addr_freq*> correlated = corr_map[addr];
				for (int i = 0; i < correlated.size(); i++) {
					predicted.insert(correlated[i]->address);
					to_visit.push(correlated[i]->address);
				}
				current_depth += 1;
			}	
			return predicted;
		}*/
		
		std::unordered_set<uint64_t> find_predicted_freqs() {
			std::queue<uint64_t> to_visit;
			to_visit.push(current_page);
			
			std::unordered_set<uint64_t> predicted;
			int current_depth = 0;
			int max_depth = pow(S, lookahead + 1);
			while (!to_visit.empty() && current_depth <= max_depth) {
				uint64_t addr = to_visit.front();
				to_visit.pop();
				
				std::vector<addr_freq*> correlated = corr_map[addr];
				for (int i = 0; i < correlated.size(); i++) {
					predicted.insert(correlated[i]->address);
					to_visit.push(correlated[i]->address);
				}
				current_depth += 1;
			}	
			return predicted;
		}

	public:
		GAC(uint64_t slots, uint64_t lookahead_amount) : S(slots),
			lookahead(lookahead_amount),
			last_accessed_page(0),
			current_page(0) {}

		~GAC(){}


		std::vector<uint64_t> find_prefetch_addrs(uint64_t full_addr) {
			set_current_values(full_addr);
			std::unordered_set<uint64_t> predicted = find_predicted_freqs();
			update_prev_correlation();
			
			std::vector<uint64_t> result_addrs= freq_to_addresses(predicted);
			update_previous_values();
			// turn a vector of structs into a vector of addresses
			return result_addrs;
		}
};

#endif // __GAC_L_H__
