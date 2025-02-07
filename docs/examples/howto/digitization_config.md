<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Generate a configuration file for the smearing digitizer

As a convenience, a simple helper script is provided to help producing JSON configuration files for smearing digitization.
The script is located in the source tree at `Examples/Algorithms/Digitization/scripts/smearing-config.py`.

Each volume configuration is one logical block consisting of the following options:

* `--digi-smear-volume=X`: Specifies the numeric identifier of the
  volume configured in the present block. This option also acts as a
  delimiter for different blocks.
* `--digi-smear-indices=X[:Y...]`: Specifies the dimensions to be configured in a comma-delimited list
* `--digi-smear-types=X[:Y...]`: Specifies the smearer type for each dimension. The following values are allowed:
  * 0: Gaussian
  * 1: Truncated gaussian
  * 2: Clipped gaussian
  * 3: Uniform
  * 4: Digital
* `--digi-smear-parameters=X[:Y...]`: Configuration parameters, 1 for Gaussian, 3 for all others (1 parameter, 2 range values)

Such blocks may be repeated as often as needed to configure many volumes, but each block has to begin with `--digi-smear-volume` and must contain all of the above arguments.

## Example

The following snippet will print a JSON string containing the
configuration for two different volumes. The first volume applies
gaussian smearing to the first two spatial dimensions and uniform
smearing to the time dimension. Correspondingly, the parameter list
for this volume has five entries: two for the simple Gaussians and
three for the uniform smearer. After this, another volume gets only
one spatial dimension smeared by a simple gaussian.

```bash
Examples/Algorithms/Digitization/scripts/smearing-config.py \
   --digi-smear-volume=8 \
   --digi-smear-indices=0:1:5 \
   --digi-smear-types=0:0:3 \
   --digi-smear-parameters=10:20:2.5:-25:25 \
   --digi-smear-volume=11 \
   --digi-smear-indices=1 \
   --digi-smear-types=0 \
   --digi-smear-parameters=12.5
```
