# ITSCAM 600 Plugins examples

The ITSCAM 600 is able to run external code as a docker container using the internal ARM 64 processor.

## Enable buildx
In order to cross-compile in docker we are going to use a tool named buildx
(documentation [here (docker docs)](https://docs.docker.com/buildx/working-with-buildx)
and [here (github)](https://github.com/docker/buildx)).

Docker currently supports
[these architectures](https://github.com/docker-library/official-images#architectures-other-than-amd64)

On linux it may be needed to enable experimental features on docker with:

```bash
export DOCKER_BUILD_KIT=1
export DOCKER_CLI_EXPERIMENTAL=enabled
```

## Create builder
Download qemu with the multi-arch target container environment and attach a builder to it.

```bash
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
docker buildx create --name itscam600-plugin-builder --driver docker-container --use
docker buildx inspect --bootstrap
# Plataforms must list 'linux/arm64'
docker buildx use itscam600-plugin-builder
```

## Examples

The follow examples was implemented to be used as base to implement your own ITSCAM 600 plugin.

### echo_server - TCP Echo Socket Server
It's a very simple plugin implementation using python. This plugin will start a socket server on a specific port and will implement a echo function.

### protocol_webservice_json - Gateway ITSCAM Protocol to WebService REST JSON
This plugin is a more complete example implemented using C/C++. It will connect on port 50.000 from TASCAM 600 and forward all vehicle captured by device and send to a Webservice REST endpoint using a JSON body.

### protocol_webservice_multipart - Gateway ITSCAM Protocol to WebService REST Multipart
This plugin is a more complete example implemented using C/C++. It will connect on port 50.000 from TASCAM 600 and forward all vehicle captured by device and send to a Webservice REST endpoint using a JSON body.