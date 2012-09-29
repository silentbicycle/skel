skel.

# Why another "simple" template program? I mean, come on, man.

This is a little standalone C program. It has a very small startup time, and the process environment makes for a decent key/value map. It's fast, I use it, thought I'd share it.

# How do I build it?

    $ make skel
    
# How do I run the tests?

    $ make test

# So, how do I use it, then?

    $ env FOO="what #{FOO} should expand to" skel TEMPLATE_FILE
    
If a template file is unspecified (or "-"), it will read the template line-by-line from `stdin`. You can escape expanders with \ , i.e., \\#{FOO} will be output as "#{FOO}" rather than getenv("FOO").

# Command line options

    -h:        print help
    -o OPENER: set opener for variable pattern (def. "#{")
    -c CLOSER: set closer for variable pattern (def. "}")
    -d FILE:   read default values from a file
    -p PATH:   path to your skeletons' closet
    -e:        abort if variable is undefined (otherwise "")

If the `-d` option is used, it looks for a file structured like:

    VAR1 the rest of the line goes in the variable
    # this is a commend
    VAR2 another variable

and uses its definitions as defaults.

Unless the `-p` option specifies a path, it will check for the template file in the following paths:

    .
    ${SKEL_CLOSET} (default: ~/.dem_bones)
    ${SKEL_SYSTEM_CLOSET} (default: /usr/local/share/skel/)
