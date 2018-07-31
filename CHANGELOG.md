# skel Changes By Release

## v0.3.0 2018-07-31

### API Changes

Instead of hardcoding the output buffer size, grow it on demand,
and significantly increase the size for all other buffers. 

Added optional output file argument.

If the template file argument is a directory, list the template
files in it and exit.


### Bug Fixes

When saving variables from the default file, save to the end of the
line, not just until the first whitespace.

Clear substitution attributes after expansion, rather than erroneously
applying them to all other substitutions on the same template line.

Don't modify environment variables directly -- copy their contents into
a buffer and modify that instead.


### Other Improvements

Added this CHANGELOG.

Moved code into `src/`, tests into `test/`, and build into `build/`.

Minor cleanup and code style changes.


## v0.2.0 2015-02-08

### Other Improvements

Add man page.

Internal cleanup.


## v0.1.2 2015-01-06

### API Changes

Add `-x` flag and execution patterns.

### Other Improvements

Added `install` and `uninstall` make targets.


## v0.1.1 2014-02-24

### Bug Fixes

Removed `-i` option (ignore templates in current directory) because
it combined badly with full paths.

Preserve escape '\'s that aren't before substitution markers.


## v0.1.0 2012-09-29

Initial public release.
