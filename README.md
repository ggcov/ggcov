ggcov
---

This is a simple GUI for browsing C test coverage data gathered
by programs instrumented with `gcc --coverage`.  Hence it's a
graphical replacement for the `gcov` program that comes with gcc.
It can also generate a static HTML report like the `lcov` program.

I wrote this program because I was sick of crappy text mode coverage
results, having been spoilt some years earlier by the PureCoverage GUI.

Using ggcov
---

To use ggcov:

- Instrument your code by building with `gcc --coverage`.  This will
  generate some `.gcno` files.
- Run your tests.  This will generate some `.gcda` files.
- Invoke ggcov with the filename of an executable, or with a source
  directory, or with one or more source (`.c`, `.cxx` etc) filenames.
  Ggcov will find and read the `.gcno`, and `.gcda` files and display
  data for you in some nice GUI windows.

Ggcov also comes with alternate ways to view coverage information.

- `ggcov-html` generates a directory full of static HTML, like `lcov`.
- `git-history-coverage` shows coverage correlated to recent git
  commits, useful for CI.
- `tggcov` emits text output like `gcov`, and has a mode to generate
  Coberatura format coverage reports, useful for CI.
- `ggcov-run` is a utility useful for running instrumented programs
  in directories other than their build directory, or on a different
  machine.
- `ggcov-web` is a PHP application for interactively displaying coverage
  data in a browser (superceded by `ggcov-html`).

Ggcov supports a powerful set of suppression primitives, so you can
avoid your coverage numbers being under-reported due to code which
you know will never execute.

Limitations
-----------

The gcc+ggcov system has several limitations and gotches of which you
should be aware.

- Gcc will add enough instrumentation to `.gcno` files for ggcov to
  tell that certain arcs between basic blocks are actually calls
  to other functions, but there isn't enough information to tell
  *which* other functions are being called, even when this is
  known at compile time.  Ggcov attempts to extract this information
  after the fact by scanning the code in object files and correlating
  that with the `.gcno` files.  This process can fail for several
  reasons, which will result in the data in the Call Graph and Call
  Butterfly windows being absent or incomplete, and the data in the
  Calls window not having the function names.  Reasons for the include:

  - the object files are missing

  - the object files are for an architecture which is not yet
    supported by ggcov for the purposes of this feature (at time
    of writing, this feature is only supported on Linux x86 and x86-64).

  - calls through function pointers or C++ virtual functions
    are not known at compile time and cannot be calculated
    using the data available to ggcov.

- Code which puts multiple basic blocks on a line may not give the
  line coverage numbers you expect.  In particular, when an entire
  loop is squashed into a single line, ggcov will report the number
  of times the loop ran plus one for each time the loop started,
  instead of the number of times the line as a whole ran.

- The Call Graph window uses a very primitive graph layout algorithm
  and may well loop or crash when given used on some programs.

Greg Banks
27 June 2020.
