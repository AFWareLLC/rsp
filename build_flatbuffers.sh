#!/bin/bash

flatc --cpp -o include/afware/rsp ./schema/scope_info.fbs
flatc --go -o ./cli/ ./schema/scope_info.fbs
