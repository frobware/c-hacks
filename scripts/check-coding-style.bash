#!/bin/bash

status=0

type -p indent > /dev/null 2>&1 || { echo "error: indent(1) is not installed"; exit 1; }

for i in $(find * -name '*.h' -or -name '*.c'); do
    [[ "$i" =~ "cutest" ]] && continue
    [[ "$i" =~ "CMakeFiles" ]] && continue
    diff -u <(cat $i) <(indent $i) > /dev/null
    if [ $? -ne 0 ]; then
        echo "indent $i"
        status=1
    fi
done

if [ $status -ne 0 ]; then
    echo "### error indent is sad." >&2
fi

exit $status
