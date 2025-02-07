#!/bin/bash

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

# We first run a single batch of jobs using the geant4 material track as an input.
# This will allow us to obtain a new material track file with the material associated with their respective surfaces.
# This file is then move to the input directory using it will allow us to speed up the following mapping by 50%
python3 ../Examples/Scripts/Python/material_mapping_optimisation.py  --numberOfJobs 40 --topNumberOfEvents 10000 --inputPath "MaterialMappingInputDir" --outputPath "MaterialMappingOutputDir" --doPloting  2>&1 | tee log/opti_log_init.txt
mv MaterialMappingOutputDir/optimised-material-map_tracks.root MaterialMappingInputDir/optimised-material-map_tracks.root

# In case of crash the databases might get corrupted. To prevent this we create a backup every 10 iteration of the optimisation script (roughly every 3 hours)
# In case of crash, it is then preferable to replace Mapping/Database by the last backup.
for j in {0..4}
do
    # Create a back up of the database
    if [ -d "Mapping/Database-backup-$j" ]; then
        rm -rf Mapping/Database-backup-$j
    fi
    mkdir Mapping/Database-backup-$j
    cp  Mapping/Database/*  Mapping/Database-backup-$j/
    for i in {0..9}
    do
        # Run a batch of optimisation jobs
        python3 ../Examples/Scripts/Python/material_mapping_optimisation.py  --numberOfJobs 40 --topNumberOfEvents 10000 --inputPath "/data/atlas/callaire/Acts/Material-Mapping-ODD" --outputPath "." --readCachedSurfaceInformation  2>&1 | tee log/opti_log_${j}_${i}.txt
        rm Mapping/Database/*.lock
        rm Mapping/Database/*.tmp
    done
done

# Now that the optimisation is over, the script is run one last time to create the optimised material map and the result plots
python3 ../Examples/Scripts/Python/material_mapping_optimisation.py  --numberOfJobs 00 --topNumberOfEvents 10000 --inputPath "/data/atlas/callaire/Acts/Material-Mapping-ODD" --outputPath "." --doPloting  2>&1 | tee log/opti_log.txt
