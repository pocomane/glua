#!/bin/sh

echo "Running some tests (note: error states are not currently tested)."

#############################################################
# Prepare directory

# Linux
# SHEXT="so"
# CC_ARCH_FLAG="-fPIC"

# Windows
SHEXT="dll"
CC_ARCH_FLAG=""

TEST_DIR="$(readlink -f "$(dirname "$0")")/tmp"
CC="gcc -std=c99 -Wall $CC_ARCH_FLAG "

rm -fR "$TEST_DIR"
mkdir "$TEST_DIR"
cd "$TEST_DIR"

#############################################################
# Compile static

$CC -o ./array_static.exe ../../*.c
strip ./array_static.exe

$CC -D'BINJECT_ARRAY_SIZE=3' -o ./tail_static.exe ../../*.c
strip ./tail_static.exe

cp ./tail_static.exe ./tail_static_bis.exe

#############################################################
# Compile shared

$CC -shared -fPIC -o ./libbinject_array.$SHEXT ../../binject.c || exit -1
$CC -o ./array_shared.exe ../../example.c -L ./ -lbinject_array || exit -1
strip ./libbinject_array.$SHEXT
strip ./array_shared.exe

cp ./array_shared.exe ./array_shared_bis.exe

export LD_LIBRARY_PATH="./"

#############################################################
# Test working

SEDSIZCOL='s:^[^ ]* *[^ ]* *[^ ]* *[^ ]* *[^ ]* *\([^ ]*\) *.*$:\1:g'

should_be() {
  if [ "$2" = "=" -a "$1" = "$3" ] ; then return ; fi
  if [ "$2" = "!=" -a "$1" != "$3" ] ; then return ; fi
  echo "TEST FAILS ! EXPECTING >>>"
  echo "$1"
  echo "<<< TO BE $2 TO >>>"
  echo "$3"
  echo "<<<"
  exit -1
}

test_working_sequence(){
  TEXT="hello world $1"

  # create text to embed
  echo "$TEXT" > ./"$1".txt || exit -1

  # embed the text
  echo "------> $1"
  ./"$1" ./"$1".txt || exit -1
  mv injed.exe ./"$1".emb || exit -1
  chmod ugo+x ./"$1".emb || exit -1

  # run the app with the embedded text
  echo "------> $1.emb"
  ./"$1".emb > ./"$1".rpt || exit -1

  # check output
  LEN="+$TEXT"
  LEN="${#LEN}"
  RES=$(cat ./"$1".rpt | tail -n 2 | head -n 1 | sed 's:^ A .. ::')
  EXP="A $LEN byte script was found (dump:)[$TEXT"
  should_be "$EXP" = "$RES"

  echo "------"
}

test_working_sequence array_static.exe
test_working_sequence tail_static.exe
test_working_sequence tail_static_bis.exe
test_working_sequence array_shared.exe
test_working_sequence array_shared_bis.exe

#############################################################
# Test Array / Tail method trhough exe size

ls -l > my_size_info.txt
grep '.exe.emb$' my_size_info.txt

ARRAY=$(grep 'array_shared.exe.emb$' my_size_info.txt | sed "$SEDSIZCOL")
ARRAYBIS=$(grep 'array_shared_bis.exe.emb$' my_size_info.txt | sed "$SEDSIZCOL")

# If the array method was chosen, the size of the exe is always the same
should_be "$ARRAY" = "$ARRAYBIS"

TAIL=$(grep 'tail_static.exe.emb$' my_size_info.txt | sed "$SEDSIZCOL")
TAILBIS=$(grep 'tail_static_bis.exe.emb$' my_size_info.txt | sed "$SEDSIZCOL")

# If the tail method was chosen, the size of the exe depend on the embeded script
should_be "$TAIL" != "$TAILBIS"

#############################################################
# Test shared

rm *.$SHEXT
./array_shared.exe.emb > array_shared.exe.empty.rpt 2> ./array_shared.exe.error.rpt

RES=$(cat array_shared.exe.empty.rpt)
should_be "" = "$RES"

#############################################################
# Print succesfull summary

echo "ALL RIGHT"

