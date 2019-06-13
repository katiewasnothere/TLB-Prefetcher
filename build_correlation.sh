#!/bin/bash

if [ "$#" -ne 4 ]; then
	echo "Incorrect # of arguments"
	echo "Usage: ./build_correlation.sh tlb_prefetcher num_cores S lookahead"
	exit
fi

BRANCH="perceptron"  # branch/*.bpred
L1D_PREFETCHER="no"   # prefetcher/*.l1d_pref
L2C_PREFETCHER="no"   # prefetcher/*.l2c_pref
TLB_PREFETCHER="${1}"
LLC_REPLACEMENT="lru"  # replacement/*.llc_repl
NUM_CORE=${2}            # tested up to 8-core system

S=${3} # The number of slots per TLB prefetcher entry
LOOKAHEAD=${4}

if [ "$S" != 2 ] 
then
	echo "Updating S variable for '${TLB_PREFETCHER}' build"
	sed -i.bak 's/\<uint64_t S = 2;\>/uint64_t S = '${S}';/g' prefetcher/${TLB_PREFETCHER}.tlb_pref
else
	echo "Using default variable for '${TLB_PREFETCHER}' build"
fi

if [ "$LOOKAHEAD" != 0 ]
then
	echo "Updating the lookahead for '${TLB_PREFETCHER}' build"
	sed -i.bak 's/\<uint64_t lookahead = 1;\>/uint64_t lookahead = '${LOOKAHEAD}';/g' prefetcher/${TLB_PREFETCHER}.tlb_pref
fi

./build.sh ${TLB_PREFETCHER} ${NUM_CORE}


sed -i.bak 's/\<uint64_t S = '${S}';\>/uint64_t S = 2;/g' prefetcher/${TLB_PREFETCHER}.tlb_pref
sed -i.bak 's/\<uint64_t lookahead = '${LOOKAHEAD}';\>/uint64_t lookahead = 1;/g' prefetcher/${TLB_PREFETCHER}.tlb_pref


