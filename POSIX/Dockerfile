FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    gcc \
    make \
    vim \
    htop \
    stress-ng \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY bm1.c bm2.c ./

# Compile both benchmarks
RUN gcc -o bm1 bm1.c -lrt -Wall -O2 && \
    gcc -o bm2 bm2.c -lrt -Wall -O2

CMD ["/bin/bash"]
