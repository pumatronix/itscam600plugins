FROM alpine:latest as build

LABEL description="Build container for Capture Notifier"

WORKDIR /src

# Install your dependencies
RUN apk update && apk add --no-cache cmake make g++ curl-dev nlohmann-json

COPY . .

RUN mkdir -p build && \
        cmake -DCMAKE_BUILD_TYPE=Release -Bbuild -H. && \
        cmake --build build --clean-first

# Prepare image that will actually run
FROM alpine:latest as runtime

LABEL description="Run container for Capture Notifier"

RUN apk update && \
        apk add --no-cache libstdc++ libcurl && \
        mkdir -p /usr/local/bin && mkdir -p /var/captures

COPY --from=build /src/build/capture_notifier /usr/local/bin/capture_notifier
COPY config.json /etc

CMD /usr/local/bin/capture_notifier
