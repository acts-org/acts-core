<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# GSF Debugger

This is a (not so) tiny tool that should ease inspecting the very complicated GSF logs.
The infrastructure is designed to be generic and extendable, so it should be not too much of an effort to adapt it e.g. to the CKF.

**Important:** This tool is probably quite unstable, especially the log parsing. However, the code is not so complicated, if you encounter problems, just fix the issue :)

## Usage

The debugger can used with a saved VERBOSE log file of a **single** trackfit:
```bash
python src/main.py --log=mylog.txt
```

Another option is to pipe it to the process:
```bash
./my_gsf_run_script.sh | python src/main.py
```

Further options can be found with
```bash
python src/main.py --help
```

## Further reading

See also in the documentation: put-link-here
