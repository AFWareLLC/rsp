#!/bin/sh

PWD=`pwd`
cd cli && go build -o ../bin/rsp && cd $PWD
