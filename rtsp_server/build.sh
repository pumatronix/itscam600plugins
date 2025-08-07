#!/usr/bin/env bash


export BUILDX_NO_DEFAULT_LOAD=true


docker buildx build --platform linux/arm64 -t rtsp_server -o type=docker,dest=- . > rtsp_server.tar