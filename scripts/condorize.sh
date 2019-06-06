#!/bin/bash
GROUP="GRAD"
PROJECT="ARCHITECTURE"
DESCR="Micro-architectural simulation"
EMAIL="kbaldauf@cs.utexas.edu"

GPU=$1
INIT_DIR=$2
SIGNATURE=$3

SCRIPT_FILE="$INIT_DIR/$SIGNATURE.sh"
CONDOR_FILE="$INIT_DIR/$SIGNATURE.condor"

echo "+Group=\"$GROUP\"" > $CONDOR_FILE
echo "+Project=\"$PROJECT\"" >> $CONDOR_FILE
echo "+ProjectDescription=\"$DESCR\"" >> $CONDOR_FILE
echo "universe=vanilla" >> $CONDOR_FILE
echo "getenv=true" >> $CONDOR_FILE
echo "Rank=Memory" >> $CONDOR_FILE
echo "notification=Error" >> $CONDOR_FILE
echo "output=CONDOR.${SIGNATURE}.OUT" >> $CONDOR_FILE
echo "error=CONDOR.${SIGNATURE}.ERR" >> $CONDOR_FILE
echo "Log=CONDOR.${SIGNATURE}.LOG" >> $CONDOR_FILE
echo "notify_user=$EMAIL" >> $CONDOR_FILE
if [ "$GPU" = true ]; then
    echo "requirements=Cuda8 && TARGET.GPUSlot && CUDAGlobalMemoryMb >= 6144" >> $CONDOR_FILE
    echo "request_GPUs=1" >> $CONDOR_FILE
    echo "+GPUJob=true && NumJobStarts == 0" >> $CONDOR_FILE 
fi
echo "initialdir=$INIT_DIR" >> $CONDOR_FILE
echo "executable=$SCRIPT_FILE" >> $CONDOR_FILE
echo "queue" >> $CONDOR_FILE
