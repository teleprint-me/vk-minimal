#!/usr/bin/env bash
set -e

DOXYGEN_CONFIG="doxy.conf"

if [[ ! -f $DOXYGEN_CONFIG ]]; then
    echo "Generating default Doxygen config..."
    doxygen -g $DOXYGEN_CONFIG
fi

echo "Generating documentation..."
doxygen $DOXYGEN_CONFIG

echo "Docs generated in docs/html/"
