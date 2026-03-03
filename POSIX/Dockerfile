FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    gcc \
    htop \
    stress-ng \
    sudo \
    && rm -rf /var/lib/apt/lists/*

ARG USERNAME=oisin_mclaughlin
RUN useradd -m -s /bin/bash ${USERNAME} && \
    echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

WORKDIR /workspace

COPY bm1.c bm2.c bm_complete.c ./

RUN gcc -o bm1 bm1.c -lrt -Wall -O2 && \
    gcc -o bm2 bm2.c -lrt -Wall -O2 && \
    gcc -o bm_complete bm_complete.c -lrt -Wall -O2

USER ${USERNAME}

CMD ["/bin/bash"]
