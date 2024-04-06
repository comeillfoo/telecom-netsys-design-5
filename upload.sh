#!/usr/bin/env bash

NS3_DIR="${NS3_DIR:-./workspace/ns-3.41}"

SCRATCH_DIR="${NS3_DIR}/scratch"

if [ -d "${SCRATCH_DIR}" ]; then
	echo "Found ${SCRATCH_DIR}"
	echo "Copying $*..."
	cp $@ "${SCRATCH_DIR}/"
fi

