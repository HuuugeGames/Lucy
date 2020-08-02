#!/bin/bash

LUCY="$(realpath ${1})"
TEST_DIR="${2}"
TMP="tmp.report"

RED="\e[1;31m"
GREEN="\e[1;32m"
NOCOLOR="\e[0m"

cd "${TEST_DIR}"
for f in ./test_*.lua; do

	if ! "${LUCY}" --dump-issues "${TMP}" "${f}" > /dev/null 2>&1; then
		result="${RED}ERROR"
	elif ! diff -q "${TMP}" "$(basename "${f}" lua)report"; then
		result="${RED}WRONG"
	else
		result="${GREEN}OK"
	fi

	echo -e "[${f}] ${result}${NOCOLOR}"
done
rm -f "${TMP}"
