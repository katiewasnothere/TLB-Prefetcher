#!/bin/bash

# Check that args are correct
if [ "$#" -ne 1 ]; then
    echo "Incorrect # of arguments: expected tlb_prefetcher policy"
    echo "Usage: ./run_cluster.sh tlb_prefetcher"
    exit
fi

# Simulation parameters
GPU=false
TRACE_PATH="/scratch/cluster/akanksha/CRCRealTraces"
CHAMP_PATH="$(pwd)/.."

BRANCH="perceptron"
L1P="no"
L2P="no"
TLB_PREFETCHER=${1}
CACHE="lru"
CORES=1

WARM_INS=0
SIM_INS=100

OUTPUT_DIR="${CHAMP_PATH}/output/spec06/${TLB_PREFETCHER}"
BINARY="${BRANCH}-${L1P}-${L2P}-${TLB_PREFETCHER}-${CACHE}-${CORES}core"

# Ensure output dir exists
if test ! -d ${OUTPUT_DIR}; then
    mkdir ${OUTPUT_DIR}
fi

# Ensure binary exists
if test ! -f ${CHAMP_PATH}/bin/${BINARY} ; then
    echo "No binary found for ${TLB_PREFETCHER}"
    echo "Build champsim with your policy before running it"
    exit
fi

# Run our cache replacement policy on each trace
while read TRACE; do
    SCRIPT_FILE="${OUTPUT_DIR}/${TRACE}.sh"
    CONDOR_FILE="${OUTPUT_DIR}/${TRACE}.condor"

    # create script file
    echo "#!/bin/bash" > ${SCRIPT_FILE}
    echo "${CHAMP_PATH}/bin/${BINARY} -warmup_instructions ${WARM_INS}000000 -simulation_instructions ${SIM_INS}000000 -hide_heartbeat -traces ${TRACE_PATH}/${TRACE}.trace.gz &> ${OUTPUT_DIR}/${TRACE}.txt" >> ${SCRIPT_FILE}
    chmod +x ${SCRIPT_FILE}

    # create condor file
    ${CHAMP_PATH}/scripts/condorize.sh ${GPU} ${OUTPUT_DIR} ${TRACE}

    # submit the condor file
    /lusr/opt/condor/bin/condor_submit ${CONDOR_FILE}
done < $CHAMP_PATH/sim_lists/spec/comp_traces.txt

#done < $CHAMP_PATH/sim_lists/spec/astar.txt 
