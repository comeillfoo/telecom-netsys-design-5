#!/usr/bin/env bash

NS3_DIR="${NS3_DIR:-workspace/ns-3.41}"

if [ -d "${NS3_DIR}" ]; then
	echo "Found ns-3 at ${NS3_DIR}"
	cd "${NS3_DIR}"
	./ns3 build
fi

