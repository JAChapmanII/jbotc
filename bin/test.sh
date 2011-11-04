#!/bin/bash

DATFILE="jbotc.dat"
LOGFILE="jbotc.log"
BIN="bin/jbot"
TESTDIR="tests"
SKIPSTART=1
SKIPATEND=0

OUTF="test.stdout"
ERRF="test.stderr"

function cleanup() {
	rm -f "$DATFILE" "$LOGFILE" "$OUTF" "$ERRF"
}

if [[ -z $1 ]]; then
	echo "Usage: $0 <test>"
	echo "Checks <test> against proper output"
	exit 1
fi

TFILE="$TESTDIR/$1.test"
RFILE="$TESTDIR/$1.res"

if [[ ! -f $TFILE ]]; then
	echo "$TFILE does not exist!"
	exit 1
fi
if [[ ! -f $RFILE ]]; then
	echo "$RFILE does not exist!"
	exit 1
fi

cat "$TFILE" | segfind "$BIN" 1> $OUTF 2> $ERRF
tail -n +$((SKIPSTART + 1)) $OUTF | head -n -$SKIPATEND | sponge $OUTF

fail=0
if !  diff "$OUTF" "$RFILE" &>/dev/null; then
	fail=1
	echo "Actual output:"
	cat "$OUTF"
	echo ""
	echo "Diff between actual and expected output:"
	diff -u "$OUTF" "$RFILE" | tail -n +4
	echo ""
fi

if [[ $(cat $ERRF | wc -l) > 0 ]]; then
	fail=1
	echo "Contents of stderr stream:"
	cat "$ERRF"
	echo ""
fi

if [[ $fail == 1 ]]; then
	echo "Test failed!"
	cat "$LOGFILE"
	echo ""
else
	echo "All OK!"
fi

cleanup

