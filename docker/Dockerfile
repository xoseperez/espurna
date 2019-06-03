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

RUN git clone -b dev https://github.com/xoseperez/espurna.git /espurna

WORKDIR /espurna/code

RUN npm install --only=dev && \
    platformio run --target clean

VOLUME ["/espurna", "/firmware"]

ENTRYPOINT ["./build.sh", "-d", "/firmware"]
CMD []
