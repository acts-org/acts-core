<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Profiling

Software profiling allows you to inspect the performance of a piece of software, seeing where the bottlenecks are and how they can be improved.
gperftools is a software profiling package. It contains a CPU profiler, thread-caching malloc library, memory leak detection tool, memory allocation profiler and pprof (discussed later). More information about gperftools and its components can be found on the project's [GitHub](https://github.com/gperftools/gperftools) and [documentation page](https://gperftools.github.io/gperftools/).

## Install gperftools

It is strongly recommended to install [libunwind](https://github.com/libunwind/libunwind) before trying to configure or install gperftools.

### Ubuntu

If you're using Ubuntu you can use the following command to install gperftools:

```
apt install google-perftools libgoogle-perftools-dev
```

### Other Systems

Alternatively, you can use the following commands to install it:

```console
$ git clone https://github.com/gperftools/gperftools
$ cd gperftools
$ git tag -l # checkout the latest release version
$ git checkout <gperftools-X.x>
$ ./autogen.sh
$ ./configure --prefix=<your/desired/install/dir>
$ make
$ make install
```

This will install gperftools in `your/desired/install/dir/lib` which is the path you should use when specifying where gperftools is, if necessary.

If you wish to install gperftools to a directory that is not one of the standard directories for libraries and therefore not findable by the `-l` compiler flag, you will need to specify the path to it with the `GPERF_INSTALL_DIR` option at build time.
Further information about installing gperftools is [here](https://github.com/gperftools/gperftools/blob/master/INSTALL).

## pprof

pprof is a tool for visualising and analysing profiling data.
An older version of pprof comes bundled with gperftools but using the newer Go version comes with several benefits: nicer looking graphs and additional options that make looking through specific sections of a program easier being among them.

### Install Go pprof (Optional)

First, you must install Go. Instructions to do so are available [here](https://go.dev/doc/install).
Optionally, you can install [Graphviz](https://www.graphviz.org/download/) to produce visualisations of profiles.

Then, run the following command to install pprof itself:

```console
$ go install github.com/google/pprof@latest
```

## Link gperftools Libraries When Compiling

The library needed to run the CPU profiler should be linked into the ACTS project using the following build option:

```
-DACTS_ENABLE_CPU_PROFILING=ON
```

Similarly, to enable the memory profiler the following build option should be used:

```
-DACTS_ENABLE_MEMORY_PROFILING=ON
```

## Alternative to Recompiling

Alternatively, you can avoid rebuilding the project by pointing the `LD_PRELOAD` environment variable to the profiler library for CPU profiling:

```
LD_PRELOAD="<path/to/libprofiler.so>" <other_options> <path/to/binary> <binary_flags>
```

You can do the same thing with the tcmalloc library for memory profiling:

```
LD_PRELOAD="<path/to/libtcmalloc.so>" <other_options> <path/to/binary> <binary_flags>
```

Using the `LD_PRELOAD` method is not recommended by the developers of gperftools so using the build options is preferable. Both CPU and memory profiling can be enabled at the same time but note that turning on memory profiling (or the heap checker) will affect performance.
Specify multiple libraries to load with `LD_PRELOAD` using a space-separated list e.g.

```
LD_PRELOAD="<path/to/first/library> <path/to/second/library>"
```

Note that these steps don't turn on profiling, they only enable it to work. The following section details how to turn it on.

## Produce a CPU Profile

To turn on CPU profiling when running an executable define the `CPUPROFILE` environment variable when executing the program:

```
CPUPROFILE=<path/to/profile> <path/to/binary> [binary args]
```

This variable specifies where the profile will be written to.
There are additional environment variables that modify the behaviour of the profiler.
[Would you like to know more](https://github.com/gperftools/gperftools)?

## Produce a Memory Profile

To turn on memory profiling use the following command:

```
HEAPPROFILE=<path/to/profile> <path/to/binary> [binary args]
```

## Run the Heap Checker

To run the heap checker for checking for memory leaks run the following command:

```
PPROF_PATH=<path/to/pprof> HEAPCHECK=normal <path/to/binary> [binary args]
```

The CPU profiler, memory profiler and heap checker can be used in tandem.

## Using pprof

### View Profile as a Graph

A graphical representation of a profile can be produced using:

```
pprof -pdf <path/to/binary> <path/to/profile> > <path/to/pdf>
```

Where `path/to/binary` is the binary is used to produce the profile in the first place.
Other output formats are available.

The following opens the graph in your web browser:

```
pprof -web <path/to/binary> <path/to/profile>
```

### Interactive Mode

To launch pprof in interactive mode use the following command:

```
pprof <path/to/binary> <path/to/profile>
```

The following command will display the top x entries by the current sorting criteria:

```
top <number>
```

To view the statistics of a function line by line use:

```
list <nameOfFunction>
```

Various options can be specified to filter, sort and set the granularity of entries.
There are also a number of other commands available.
Read more about pprof [here](https://github.com/google/pprof).
