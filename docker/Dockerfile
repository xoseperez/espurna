FROM debian:stable-slim

# silence env warnings
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    python3-minimal \
    python3-pip \
    python3-setuptools \
    ca-certificates \
    nodejs \
    npm \
    git 

# NOTE: there is currently a compability warning with the target versions of Node vs. NPM - using latest until something is identified as the target version
RUN rm -rf /var/lib/apt/lists/* \
    && npm install npm@latest -g

# NOTE: Python module for pip requires upgrade adhead of PIO install to work around defects
RUN python3 -m pip install -U pip

# now that pip is upgraded, we can install platformio without it crashing
RUN python3 -m pip install platformio

# Create the working directories for where to copy code for a clean build and for output
RUN mkdir -p /espurna && \
    mkdir -p /firmware 

# Copy in source code 
COPY . /espurna

# update our working director so that ./build.sh will execute correctly
WORKDIR /espurna/code

RUN npm install --only=dev && \
    platformio lib install && \
    platformio run --target clean

VOLUME ["/espurna", "/firmware"]

# finally execute what we're here for
ENTRYPOINT ["./build.sh", "-d", "/firmware"]
CMD []
