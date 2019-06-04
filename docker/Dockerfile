FROM debian:buster-slim

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    python2-minimal \
    python-pip \
    python-setuptools \
    ca-certificates \
    nodejs \
    npm \
    git \
    && rm -rf /var/lib/apt/lists/* \
    && npm install -g npm \
    && pip install "platformio>=3.6,<3.7"

ARG UID=1000
ARG GID=1000

RUN groupadd --gid $GID worker && \
    useradd \
    --uid $UID \
    --gid worker \	
    --shell /bin/bash \
    --create-home worker

RUN mkdir -p /espurna && \
    mkdir -p /firmware && \
    chown -R worker:worker /espurna && \
    chown -R worker:worker /firmware

COPY --chown=worker:worker . /espurna

USER worker

WORKDIR /espurna/code

RUN npm install --only=dev && \
    platformio run --target clean

VOLUME ["/espurna", "/firmware"]

ENTRYPOINT ["./build.sh", "-d", "/firmware"]
CMD []
