#!/bin/bash

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

#
# This script runs the propagation test with different steppers and different pT bins
#
# arguments are:
#  $

time_stamp=`date +%s%N`
run_directory=propagation_timing_${time_stamp}
mkdir ${run_directory}
cd ${run_directory}

output_file='output.log'
#magfield='--bf-values 0 0 2'
magfield='--bf-map ../ATLASBField_xyz.root'
#bfieldtype='constfield'

echo "***************************************" > ${output_file}
echo "* Test: $1" >> ${output_file}
echo "* Events: $2" >> ${output_file}
echo "* Tests/event: $300" >> ${output_file}
echo "***************************************" >> ${output_file}
echo "*"
echo "* job | stepper | pt" >> ${output_file}

jobID=0

# Loop over the Pt bins
for pt in 0.1 0.5 1.0 2.0 5.0 10.0 100.0 ; do
  # Loop over the stepper
  for stepper in {0..2} ; do

    # Compute the name of the example executable
    executable="ActsExamplePropagation$1 -n$2  ${magfield} --prop-ntests $3 -j $4 --prop-pt-range ${pt} ${pt} --prop-stepper ${stepper} --output-root"

    echo "${jobID}, ${stepper}, ${pt}" >> ${output_file}
    eval ${executable}

    # Archive with Job ID
    mv timing.tsv timing_${jobID}.tsv
    mv propagation-steps.root propagation_steps_${jobID}.root

    # JobID
    let "jobID++"

  done
done
