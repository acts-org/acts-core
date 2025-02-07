#!/bin/bash

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

#
# This script runs the CombinatorialKalmanFitter(CKF) timing test at different pileup (mu) bins using ttbar events
# To run the test:./CKF_timing_vs_mu.sh -d <detector> -b <bFieldMap> -n <numEvents>
# In default, it will run with Generic detector in constant B field using 10 event per mu bin
#

help()
{
   echo ""
   echo "Usage: $0 -d <detector> -b <bFieldMap> -t <numTracksPerEvent> -n <numEvents>"
   echo -e "\t-d The detector type, either 'Generic' or 'DD4hep'. Optional. In default 'Generic'"
   echo -e "\t-x The '.xml' for DD4hep detector input. Required if the detector is 'DD4hep'. In default empty"
   echo -e "\t-b The '.txt' or '.root' file for B Field map. Optional. In default using constant BField: (0, 0, 2)"
   echo -e "\t-n The number of events. Optional. In default: 1"
   exit 1 # Exit script after printing help
}

if [ ! -f "ActsExampleFatrasGeneric" ]; then
  echo Please run this script under the directory where the executables are located
  exit 1
fi

#Default number of tracks/event and event
detector=Generic
numEvents=1
# Get parameters
while getopts "d:x:b:n:" opt
do
   case "$opt" in
      d ) detector="$OPTARG" ;;
      x ) dd4hepInput="$OPTARG" ;;
      b ) bFieldMap="$OPTARG" ;;
      n ) numEvents="$OPTARG" ;;
      ? ) help ;; # Print help in case unknown parameter
   esac
done

#check input for DDhep input
if [ "${detector}" == DD4hep ]; then
   if [ -z "${dd4hepInput}" ]; then
      echo "Empty input for --dd4hep-input. A file like $<source>/thirdparty/OpenDataDetector/xml/OpenDataDetector.xml must be provided. Have to exit."
      exit 1
   fi
   if [ ! -f "${dd4hepInput}" ]; then
      echo "The ${dd4hepInput} does not exist! Have to exit."
      exit 1
   fi
   dd4hep_input="--dd4hep-input=${dd4hepInput}"
fi

# Check input for B field
bField='--bf-value 0 0 2'
if [ -z "${bFieldMap}" ]; then
   echo "bFieldMap is empty. Will use constant B Field (0, 0, 2) then."
elif [ -f "$bFieldMap" ] ; then
   echo "Input bField map: $bFieldMap "
   bField='--bf-map ${bFieldMap}'
else
   echo "Input file for bField map file does not exist!"
   exit 1
fi

time_stamp=`date +%F%T`
exe_dir=$PWD
run_dir=CKF_timing_${time_stamp}

mkdir ${run_dir}
cd ${run_dir}

output_file='output.log'
echo "*****CombinatorialKalmanFilter timing test vs. <mu>*****" > ${output_file}
echo "Test Detector: ${detector}" >> ${output_file}
echo "BField: ${bField}" >> ${output_file}
echo "Events: ${numEvents}" >> ${output_file}
echo "****************************************" >> ${output_file}
echo "*"
echo "* job | mode | mu " >> ${output_file}

jobID=0

# Loop over the pileup bins
for mu in 0 50 100 150 200 250 300 ; do
    #Run ttbar events generation
    gen="${exe_dir}/ActsExamplePythia8  --events=${numEvents}  --output-dir=data/gen/ttbar_e${numEvents}_mu${mu} --output-csv=1 --rnd-seed=42 --gen-cms-energy-gev=14000 --gen-hard-process=Top:qqbar2ttbar=on --gen-npileup=${mu}"
    echo ${gen}
    eval ${gen}

    # Run sim
    sim="${exe_dir}/ActsExampleFatras${detector} ${dd4hep_input} ${bField} --select-pt-gev '0.1:' --select-eta '-3:3' --fatras-pmin-gev 0.1 --remove-neutral 1 --input-dir=data/gen/ttbar_e${numEvents}_mu${mu} --output-csv=1 --output-dir=data/sim_${detector}/ttbar_e${numEvents}_mu${mu}"
    echo ${sim}
    eval ${sim}

    # Loop over the combinatorial/sequential mode (different source link selection criteria)
    for mode in {0..1} ; do
      # Run reco
      if [[ $mode -eq 0 ]]; then
        reco="${exe_dir}/ActsExampleCKFTracks${detector} ${dd4hep_input} ${bField} -j 1 --input-dir=data/sim_${detector}/ttbar_e${numEvents}_mu${mu}  --output-dir=data/reco_${detector}/ttbar_e${numEvents}_mu${mu}_m${mode}"
      else
        reco="${exe_dir}/ActsExampleCKFTracks${detector} ${dd4hep_input} ${bField} -j 1 --input-dir=data/sim_${detector}/ttbar_e${numEvents}_mu${mu} --ckf-slselection-chi2max 10  --ckf-slselection-nmax 1 --output-dir=data/reco_${detector}/ttbar_e${numEvents}_mu${mu}_m${mode}"
      fi
      echo $reco
      eval ${reco}

      # Archive with Job ID
      mv data/reco_${detector}/ttbar_e${numEvents}_mu${mu}_m${mode}/timing.tsv timing_${jobID}.tsv
      # Extract the track finding time
      ckf_time_str=`grep "Algorithm:TrackFindingAlgorithm" timing_${jobID}.tsv | awk '{print $3}'`
      # Make sure the track finding time is fixed-point for calculation with bc
      ckf_time_per_event=$(echo ${ckf_time_str} | awk '{printf("%.10f\n", $1)}')
      echo "${jobID}, ${mode}, ${mu}, ${ckf_time_per_event}" >> ${output_file}

      # JobID
      let "jobID++"
  done
done
