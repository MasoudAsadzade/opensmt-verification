#!/bin/bash

if [ $# != 1 ]; then
    echo "Usage: $0 <path-to-opensmt>"
    exit 1
fi

opensmt=$1;

echo "Running model validation tests"

scriptdir=$(cd $(dirname "$0"); pwd)
scrambler=${scriptdir}/../env/bin/scrambler
validator=${scriptdir}/../env/bin/ModelValidator

errors=false

for file in ${scriptdir}/../instances/*.smt2; do
    ${scriptdir}/run-and-validate.sh \
        -o ${opensmt} \
        -s ${scrambler} \
        -c ${validator} \
        ${file} || errors=true
done

if [ $errors == "true" ]; then
    exit 1;
fi
