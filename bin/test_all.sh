#!/bin/bash

TESTDIR="tests"
fail=0
for tst in $(ls -1 "$TESTDIR" | sed 's/\..*$//' | sort -u | grep -v 'fail'); do
	if ! ./bin/test.sh $tst &>/dev/null; then
		fail=1
		echo "$tst fails!"
		./bin/test.sh $tst
		echo ""
	fi
done

if [[ $fail == 0 ]]; then
	echo "Everything a-OK!"
fi

