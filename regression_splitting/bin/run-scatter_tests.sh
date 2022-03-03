#!/bin/bash
get_abs_filename() {
  echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
}

echo "This is the script for running regression tests for scatter splitter"
echo " - date: $(date '+%Y-%m-%d at %H:%M.%S')"
echo " - host name $(hostname -f)"
echo " - script path: $(get_abs_filename $0)"

while [ $# -gt 0 ]; do
    case $1 in
        -h|--help)
            echo "${usage}"
            exit 1
            ;;
        -*)
            echo "Error: invalid option '$1'"
            exit 1
            ;;
        *)
            break
    esac
    shift; shift
done

RESULTCHECKER=./check-scatter_result.py
opensmt=$1
error=false
tmpfolder=log-$(date '+%Y-%m-%d')

if [ -d ${tmpfolder} ]
then
    rm -rf ${tmpfolder}
else
    mkdir ${tmpfolder}
fi

function run_opensmt () {
    dir=$1
    name=$2
    expected=$3

    echo ${opensmt} $dir $name
    echo $tmpfolder/$name.out
    echo $tmpfolder/$name.err.tmp

    sh -c "ulimit -St 60; ${opensmt} $dir/$name > $tmpfolder/$name.out 2>$tmpfolder/$name.err.tmp" 2>/dev/null
    grep -v '^;' $tmpfolder/$name.err.tmp > $tmpfolder/$name.err
    echo ${tmpfolder}/${name}.out
    diff -q ${tmpfolder}/${name}.out $expected

    if [ $? != 0 ]; then
        echo "stdout differs for benchmark $file";
        error=true;
        diff ${tmpfolder}/${name}.out $expected
    fi
    diff -q ${tmpfolder}/${name}.err ""
    if [ $? != 0 ]; then
        echo "stderr differs for benchmark $file";
        error=true;
        diff ${tmpfolder}/${name}.err ""
    fi

    echo $tmpfolder/$name.out ${expected}
    ${RESULTCHECKER} $tmpfolder/$name.out ${expected}
    if [[ $? != 0 ]]; then
        echo "Error in result on $tmpfolder/$name"
        error=false
        return 1
    fi
    return 0
}

for file in $(find ../scatter_instances -name '*.smt2' |sort); do

    run_opensmt $(dirname $file) $(basename $file) unknown

done


if [[ ${error} == true ]]; then
    rm -rf ${tmpfolder}
    exit 0;
else
    exit 1;
fi
