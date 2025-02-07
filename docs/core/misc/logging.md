<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Logging

The ACTS logging facility supports several severity levels which allow you to
control the amount of information displayed at run-time. Logger objects can
easily be created using the {func}`Acts::getDefaultLogger` function which
should be sufficient to get you started. In case you need more customized debug
output, you can make use of the output decorators defined in
`Acts::Logging` or even write your own implementation of
{class}`Acts::Logging::OutputDecorator`. In order to add debug messages to your
program, you should use the provided macros for the different severity levels:

```cpp
ACTS_VERBOSE(...);
ACTS_DEBUG(...);
ACTS_INFO(...);
ACTS_WARNING(...);
ACTS_ERROR(...);
ACTS_FATAL(...);
```

These macros correspond to the available log levels:

:::{doxygenenum} Acts::Logging::Level
:outline:
:::


The macros require that a function `logger` returning a {class}`Acts::Logger`
object is available in the scope in which the macros are used:

```cpp
const Logger& logger() const;
```

Inside classes
containing an {class}`Acts::Logger` object as member variable, this could be
achieved by providing a private class method called `logger()` (for an example
see e.g.  {func}`Acts::CylinderVolumeBuilder::logger`). Inside free functions
or member methods with local logger objects, the macros are also usable, since
{class}`Acts::Logger` is callable and returns a reference to itself.

Code example illustrating the usage:

:::{doxygenfunction} Acts::getDefaultLogger
:::

```cpp
#include <fstream>
#include <memory>

#include "Acts/Utilities/Logger.hpp"

void myFunction() {
  // open the logfile
  std::ofstream logfile("log.txt");
  // set up a logger instance for >= INFO messages, streaming into the log file
  std::unique_ptr<const Acts::Logger> logger
      = Acts::getDefaultLogger("MyLogger", Acts::Logging::INFO, &logfile);
  // make sure the ACTS debug macros can work with your logger
  ACTS_VERBOSE("This message will not appear in the logfile.");
  ACTS_INFO("But this one will: Hello World!");
  // do not forget to close the logfile
  logfile.close();
}
```

## Logger integration

In case you are using ACTS in another framework which comes with its own
logging facility (e.g. Gaudi) you can pipe the logging output from ACTS
tools and algorithms to your framework's logging system by supplying different
implementations of:

- {class}`Acts::Logging::OutputFilterPolicy` (for mapping logging levels)
- {class}`Acts::Logging::OutputPrintPolicy` (for passing the Acts output
  to your internal logging system)

There are two approaches to logger integration:

1. [](override_deflog).
   This has the downside that log levels cannot be controlled from top-level
   experiment specific code. This means that it is non-trivial to steer the log
   level of an e.g. Gaudi algorithm via the `OutputLevel` property, and have
   the ACTS code respect this log level. It is therefore now **discouraged** to
   use this approach.

    :::{note}
    ACTS code has iteratively moved to not construct loggers via
    {func}`Acts::getDefaultLogger` as much as possible, in favor of using a
    const-reference to {class}`Acts::Logger`. The latter can be defaulted to a
    dummy logger using {func}`Acts::getDummyLogger`. It is more suitable to
    pass into functions that might be called from other ACTS functions (rather
    than construction a local logger via `getDefaultLogger`, or creating logger
    instances on the fly).
    :::

2. Passing logger instances to high level components, and rely on ACTS code to
   pass them into lower level classes / functions.


(override_deflog)=
### Overriding `Acts::getDefaultLogger`

:::{attention}
Using this mechanism is now **discouraged** for integration with an experiment
framework.
:::

Since ACTS makes extensive use of {func}`Acts::getDefaultLogger` to provide
sufficient information for debugging, you might want to provide a modified
implementation of this function (using your output filter and printing
policies) to also pipe this output to your framework. You can use the following
approach using the possibility to inject custom code by preloading shared
libraries with `LD_PRELOAD`. You need to provide an appropriate implementation
for a function of the following signature into a separate source file and
compile it in a shared library


```cpp
namespace Acts {
std::unique_ptr<const Logger> getDefaultLogger(const std::string&,
                                               const Logging::Level&,
                                               std::ostream*);
}
```

Then you can run your executable, which uses ACTS tools and algorithms, in
the following way (tested under Unix)

```console
$ LD_PRELOAD=<YOUR_SHARED_LIBRARY> path/to/your/executable
```

## Logging thresholds

Generally, log levels in ACTS are only of informative value: even
{enumerator}`Acts::Logging::Level::ERROR` and {enumerator}`Acts::Logging::Level::FATAL` will only print a
messages, **and not terminate execution**.

This is desirable in an experiment context, where jobs should not immediately
terminate when ACTS encounters something that is logged as an error.  In a test
context, however, this behavior is not optimal: the tests should ensure in
known configurations errors do not occur, or only in specific circumstances. To
solve this, ACTS implements an optional log *threshold* mechanism.

The threshold mechanism is steered via two CMake options:
`ACTS_ENABLE_LOG_FAILURE_THRESHOLD` and `ACTS_LOG_FAILURE_THRESHOLD`. Depending
on their configuration, the logging can operate in three modes:

1. **No log failure threshold** exists, log levels are informative only. This is
   the default behavior.
2. A **compile-time log failure threshold** is set. If
   `ACTS_ENABLE_LOG_FAILURE_THRESHOLD=ON` and
   `ACTS_LOG_FAILURE_THRESHOLD=<LEVEL>` are set, the logger code will compile
   in a fixed check if the log level of a particular message exceeds `<LEVEL>`.
   If that is the case, an exception of type {class}`Acts::Logging::ThresholdFailure` is
   thrown.
3. A **runtime log failure threshold** is set. If only
   `ACTS_ENABLE_LOG_FAILURE_THRESHOLD=ON` and no fixed threshold level is set,
   the logger code will compile in a check of a global runtime threshold
   variable.

:::{note}
If only `ACTS_LOG_FAILURE_THRESHOLD` is set,
`ACTS_ENABLE_LOG_FAILURE_THRESHOLD` will be set automatically, i.e. a
compile-time threshold will be set
:::

Two functions exist to interact with the failure threshold:

:::{doxygenfunction} Acts::Logging::getFailureThreshold
:::

:::{doxygenfunction} Acts::Logging::setFailureThreshold
:::
