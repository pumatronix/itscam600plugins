

## Enable buildx
In order to cross-compile in docker we are going to use a tool named buildx
(documentation [here (docker docs)](https://docs.docker.com/buildx/working-with-buildx)
and [here (github)](https://github.com/docker/buildx)).

Docker currently supports
[these architectures](https://github.com/docker-library/official-images#architectures-other-than-amd64)

On linux it may be needed to enable experimental features on docker with:

```bash
export CKER_BUILD_KIT=1
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

## Build

```bash
./build.sh
```
