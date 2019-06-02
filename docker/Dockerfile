FROM debian:buster-slim

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y python2-minimal \
    python-pip \
    nodejs \
    npm \
    git \
    libc-bin \
    bash \
    && npm install -g npm \
    && pip install platformio

RUN groupadd --gid 1000 worker && \
    useradd \
    --uid 1000 \
    --gid worker \
    --shell /bin/bash \
    --create-home worker

RUN mkdir -p /espurna && \
    mkdir -p /firmware && \
    chown -R worker:worker /espurna && \
    chown -R worker:worker /firmware

USER worker

RUN git clone -b dev https://github.com/xoseperez/espurna.git /espurna && \
    cd /espurna/code && \
    npm install --only=dev && \
    platformio run --target clean

VOLUME ["/espurna", "/firmware"]

WORKDIR /espurna/code

COPY docker-entrypoint.sh /
ENTRYPOINT ["./build.sh", "-d", "/firmware"]
CMD []
