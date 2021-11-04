#!/usr/bin/env bash


export BUILDX_NO_DEFAULT_LOAD=true


docker buildx build --platform linux/arm64 -t echo_server -o type=docker,dest=- . > echo_server.tar