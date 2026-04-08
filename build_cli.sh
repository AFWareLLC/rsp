#!/bin/sh
set -e

flatc --go -o ./cli/ ./schema/scope_info.fbs

PWD=`pwd`
cd cli && go build -o ../bin/rsp && cd $PWD
