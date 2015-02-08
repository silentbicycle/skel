# skel

skel reads a template from stdin or a file given on the command line,
replacing expansion patterns with the contents of variables from the
environment.

For example, 

    $ skel TEMPLATE_FILE

will cat TEMPLATE_FILE, replacing instances of `#{HOME}` with the
contents of `#{HOME}` in the environment.


# Why another "simple" template program? I mean, come on, man.

This is a little standalone C program. It has a very small startup time,
and the process environment makes for a decent key/value map. It's fast,
tiny, and doesn't depend on anything.

I use it, thought I'd share it.


# How do I build it?

    $ make skel

To install it,

    $ make install
    

# How do I run the tests?

    $ make test


# So, how do I use it, then?

    $ env FOO="what #{FOO} should expand to" skel TEMPLATE_FILE
    
If a template file is unspecified (or "-"), it will read the template
line-by-line from `stdin`. You can escape expanders with \ , i.e.,
\\#{FOO} will be output as "#{FOO}" rather than getenv("FOO").


# Expansions

An expansion is a pattern of the form `#{:ATTR:VARNAME:DEFAULT}` that
appears in the template stream. The attributes and default are optional
(so just `#{HOME}` works). The expansion will be replaced with the value
of `VARNAME` in the shell environment, as modified by the attributes. If
the variable is undefined, then the default will be used (if given),
otherwise the expansion will be printed as "#{VARNAME}".

Variable names are limited to alphanumeric characters and `_`. The
expansion opener and closer strings can be changed with `-o` and `-c`
(they default to `#{` and `}`), but cannot contain `:`.

## Attributes

    l: convert to lowercase
    u: convert to uppercase
    n: eliminate trailing newline
    x: Pass expansion body to shell and insert output

The 'x' attribute requires `skel -x` to avoid accidentally running
dangerous commands, and cannot be defaulted.

For example, `#{:xn:date +%Y}` will expand to the current year (without
a trailing newline), and `#{:l:PROJNAME}` & `#{:u:PROJNAME}` would
expand to the lowercase and uppercase versions of `${PROJNAME}`.


# Command line options

    -h:        print help
    -o OPENER: set opener for expansion (def. "#{")
    -c CLOSER: set closer for expansion (def. "}")
    -d FILE:   read default values from a file
    -p PATH:   path to your skeletons' closet
    -e:        abort if variable is undefined (otherwise "#{VARNAME}")
    -x:        exec expansions with 'x' attribute and insert result.
               Example: `echo '#{:xn:date +%Y}' | skel -x` => "2015".
               (Off by default to avoid unexpected commands.)

If the `-d` option is used, it looks for a file structured like:

    VAR1 the rest of the line goes in the variable
    # this is a comment
    VAR2 another variable

and add its definitions to the environment. If putting everything in one
file is getting unwieldy, then [denv] may be of interest.

[denv]: https://github.com/silentbicycle/denv

Unless the `-p` option specifies a path, it will check for the template
file in the following paths, if present:

    .
    ${SKEL_CLOSET} (default: ~/.dem_bones)
    ${SKEL_SYSTEM_CLOSET} (default: /usr/local/share/skel/)
