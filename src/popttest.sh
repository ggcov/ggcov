#!/bin/sh

while read args ; do
    echo "===src/popttest $args"
    src/popttest $args 2>&1
done <<EOF

-b
--bool
-b -b
-b -S
--bool --spool
-bS
-s
-s FOO
--string FOO
--string=FOO
-b -S -s FOO
-bS -s FOO
-bSSb -s FOO
-bSSb -s FOO -R BAR
-bSSb -sR FOO BAR
--string FOO --rope BAR
--fmeh
-z
-R
foo
foo bar
-b foo bar
-b -- foo
-b -- -R foo
EOF
