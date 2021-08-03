#!/usr/bin/env bash

export BUILDX_NO_DEFAULT_LOAD=true

docker buildx build --platform linux/arm64 -t capture_notifier -o type=docker,dest=- . > capture_notifier.tar
