#!/bin/bash

DEFFILE="defines.h"
SEED="167896"

if [[ ! -f $DEFFILE ]]; then
	echo "The definition file: $DEFFILE: does not exist"
	exit 1
fi

tmp="$(mktemp)"

sed -nr 's/^#define (.*) (.*)$/\1=\2/p' "$DEFFILE" > "$tmp"
source "$tmp"
rm "$tmp"

BIN="bin/$jbotBinary"
TESTDIR="tests"
SKIPSTART=1
SKIPATEND=0

OUTF="test.stdout"
ERRF="test.stderr"

function cleanup() {
	rm -f "$dumpFileName" "$logFileName" "$OUTF" "$ERRF" \
		"$markovDumpFileName" "$regexDumpFileName"
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

tmpTFILE="$(mktemp)"
tmpRFILE="$(mktemp)"

cp "$TFILE" "$tmpTFILE"
cp "$RFILE" "$tmpRFILE"

sed -i "s/NICK/$nick/g" "$tmpTFILE" "$tmpRFILE"
sed -i "s/OWNER/$owner/g" "$tmpTFILE" "$tmpRFILE"
sed -i "s/#CHANNEL_NAME/$chan/g" "$tmpTFILE" "$tmpRFILE"


cat "$tmpTFILE" | segfind "$BIN" $SEED 1> $OUTF 2> $ERRF
tail -n +$((SKIPSTART + 1)) $OUTF | head -n -$SKIPATEND | sponge $OUTF

fail=0
if !  diff "$OUTF" "$tmpRFILE" &>/dev/null; then
	fail=1
	echo "Actual output:"
	cat "$OUTF"
	echo ""
	echo "Diff between actual and expected output:"
	diff -u "$OUTF" "$tmpRFILE" | tail -n +4
	echo ""
fi

rm "$tmpTFILE" "$tmpRFILE"

if [[ $(cat $ERRF | wc -l) > 0 ]]; then
	fail=1
	echo "Contents of stderr stream:"
	cat "$ERRF"
	echo ""
fi

if [[ $fail == 1 ]]; then
	echo "Test failed!"
	cat "$logFileName"
	echo ""
else
	echo "All OK!"
fi

cleanup
exit $fail

