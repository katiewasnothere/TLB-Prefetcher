#!/bin/bash

# Check that args are correct
if [ "$#" -ne 2 ]; then
    echo "Incorrect # of arguments: expected tlb_prefetcher policy and num cores"
    echo "Usage: ./build.sh tlb_prefetcher num_cores"
    exit
fi

# Build the executable (with perceptron BP, no prefetchers, 1 core)
BRANCH="perceptron"  # branch/*.bpred
L1D_PREFETCHER="no"   # prefetcher/*.l1d_pref
L2C_PREFETCHER="no"   # prefetcher/*.l2c_pref
TLB_PREFETCHER="${1}"
LLC_REPLACEMENT="lru"  # replacement/*.llc_repl
NUM_CORE=${2}            # tested up to 8-core system
HEADROOM="0"

############## Some useful macros ###############
BOLD=$(tput bold)
NORMAL=$(tput sgr0)

embed_newline()
{
   local p="$1"
   shift
   for i in "$@"
   do
      p="$p\n$i"         # Append
   done
   echo -e "$p"          # Use -e
}
#################################################

# Sanity check
if [ ! -f ./branch/${BRANCH}.bpred ] || [ ! -f ./prefetcher/${L1D_PREFETCHER}.l1d_pref ] || [ ! -f ./prefetcher/${L2C_PREFETCHER}.l2c_pref ] || [ ! -f ./prefetcher/${TLB_PREFETCHER}.tlb_pref ] || [ ! -f ./replacement/${LLC_REPLACEMENT}.llc_repl ]; then
	echo "${BOLD}Possible Branch Predictor: ${NORMAL}"
	LIST=$(ls branch/*.bpred | cut -d '/' -f2 | cut -d '.' -f1)
	p=$( embed_newline $LIST )
	echo "$p"

	echo "${BOLD}Possible L1D Prefetcher: ${NORMAL}"
	LIST=$(ls prefetcher/*.l1d_pref | cut -d '/' -f2 | cut -d '.' -f1)
	p=$( embed_newline $LIST )
	echo "$p"

	echo
	echo "${BOLD}Possible L2C Prefetcher: ${NORMAL}"
	LIST=$(ls prefetcher/*.l2c_pref | cut -d '/' -f2 | cut -d '.' -f1)
	p=$( embed_newline $LIST )
	echo "$p"

	echo
	echo "${BOLD}Possible TLB Prefetcher: ${NORMAL}"
	LIST=$(ls prefetcher/*.tlb_pref | cut -d '/' -f2 | cut -d '.' -f1)
	p=$( embed_newline $LIST )
	echo "$p"

	echo
	echo "${BOLD}Possible LLC Replacement: ${NORMAL}"
	LIST=$(ls replacement/*.llc_repl | cut -d '/' -f2 | cut -d '.' -f1)
	p=$( embed_newline $LIST )
	echo "$p"
	exit
fi

# Check for running headroom study
if [ "$HEADROOM" != "0" ]
then
	#we're running the headroom study, set the macro definition
	echo "${BOLD}Building compulsory HEADROOM study Champsim... ${NORMAL}"
	sed -i.bak 's@//\x23define HEADROOM\>@#define HEADROOM@g' inc/champsim.h
fi 

# Check for multi-core
if [ "$NUM_CORE" != "1" ]
then
    echo "${BOLD}Building multi-core ChampSim...${NORMAL}"
    sed -i.bak 's/\<NUM_CPUS 1\>/NUM_CPUS '${NUM_CORE}'/g' inc/champsim.h
	sed -i.bak 's/\<DRAM_CHANNELS 1\>/DRAM_CHANNELS 2/g' inc/champsim.h
	sed -i.bak 's/\<DRAM_CHANNELS_LOG2 0\>/DRAM_CHANNELS_LOG2 1/g' inc/champsim.h
else
    echo "${BOLD}Building single-core ChampSim...${NORMAL}"
fi
echo

# Change prefetchers and replacement policy
cp branch/${BRANCH}.bpred branch/branch_predictor.cc
cp prefetcher/${L1D_PREFETCHER}.l1d_pref prefetcher/l1d_prefetcher.cc
cp prefetcher/${L2C_PREFETCHER}.l2c_pref prefetcher/l2c_prefetcher.cc
cp prefetcher/${TLB_PREFETCHER}.tlb_pref prefetcher/tlb_prefetcher.cc
cp replacement/${LLC_REPLACEMENT}.llc_repl replacement/llc_replacement.cc

# Build
mkdir -p bin
rm -f bin/champsim
make clean
make

# Sanity check
echo ""
if [ ! -f bin/champsim ]; then
    echo "${BOLD}ChampSim build FAILED!${NORMAL}"
    echo ""
    exit
fi

echo "${BOLD}ChampSim is successfully built"
echo "Branch Predictor: ${BRANCH}"
echo "L1D Prefetcher: ${L1D_PREFETCHER}"
echo "L2C Prefetcher: ${L2C_PREFETCHER}"
echo "TLB Prefetcher: ${TLB_PREFETCHER}"
echo "LLC Replacement: ${LLC_REPLACEMENT}"
echo "Cores: ${NUM_CORE}"
BINARY_NAME="${BRANCH}-${L1D_PREFETCHER}-${L2C_PREFETCHER}-${TLB_PREFETCHER}-${LLC_REPLACEMENT}-${NUM_CORE}core"
echo "Binary: bin/${BINARY_NAME}${NORMAL}"
echo ""
mv bin/champsim bin/${BINARY_NAME}


# Restore to the default configuration
sed -i.bak 's/\<NUM_CPUS '${NUM_CORE}'\>/NUM_CPUS 1/g' inc/champsim.h
sed -i.bak 's/\<DRAM_CHANNELS 2\>/DRAM_CHANNELS 1/g' inc/champsim.h
sed -i.bak 's/\<DRAM_CHANNELS_LOG2 1\>/DRAM_CHANNELS_LOG2 0/g' inc/champsim.h

if [ "$HEADROOM" != "0" ]
then
	sed -i.bak 's@\x23define HEADROOM\>@//#define HEADROOM@g' inc/champsim.h
fi

cp branch/perceptron.bpred branch/branch_predictor.cc
cp prefetcher/no.l1d_pref prefetcher/l1d_prefetcher.cc
cp prefetcher/no.l2c_pref prefetcher/l2c_prefetcher.cc
cp prefetcher/no.tlb_pref prefetcher/tlb_prefetcher.cc
cp replacement/lru.llc_repl replacement/llc_replacement.cc
