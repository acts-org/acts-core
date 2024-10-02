(howto_format)=

# Source code formatting

## C++ formatting: `clang-format`

Code formatting is handled by
[`clang-format`](https://clang.llvm.org/docs/ClangFormat.html). A configuration
file is available in the repository root at `.clang-format` and should be used
to automatically format the code. Many editors / IDEs support `clang-format`
and also format-on-save actions.

The ACTS CI system will automatically check code formatting using the provided
`clang-format` configuration and will notify incompatible formatting.

To simplify this, a script located in `CI/check_format` can be used like:

```console
$ CI/check_format $SOURCE_DIR
```

In some cases, different `clang-format` versions will result in slightly
different outputs. In many cases, this is accepted by the CI. However, it is
recommended to use the same major version of `clang-format` to perform local
formatting. Options to obtain a compatible `clang-format` version
are to use your package manager (e.g. Ubuntu distributions usually offer a set of
versions to install), or to use statically linked binaries from
[here](https://github.com/muttleyxd/clang-tools-static-binaries)[^1] and use them with:

```console
CLANG_FORMAT_BINARY=<path/to/clang-format> CI/check_format $SOURCE_DIR
```

You can also download the required changes by clicking on *Summary* on the top left-hand
portion of the CI job and scrolling down to the bottom of the page (see *Changed*).
However, it is suggested to run the `CI/check_format` locally before committing, to not
clog the shared resources with repeated checks.

## Python formatting

Formatting of the Python source code uses the library
[`black`](https://github.com/psf/black). To run it, you can locally install the
`black` package. You can use `pip` to install it:

```console
$ pip install black
$ black <source>
```

:::{tip}
It is **strongly recommended** to use a [virtual
environment](https://realpython.com/python-virtual-environments-a-primer/) for
this purpose! For example, run

```console
$ python -m venv venv
$ source venv/bin/activate
```

and then install and use black. You can also use a tool like
[`pipx`](https://github.com/pypa/pipx) to simplify this.
:::

[^1]: This repository is external to the ACTS project, so proceed with caution!
