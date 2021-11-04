#!/usr/bin/env bash

export BUILDX_NO_DEFAULT_LOAD=true

docker buildx build --platform linux/arm64 -t protocol_webservice_json -o type=docker,dest=- . > protocol_webservice_json.tar
