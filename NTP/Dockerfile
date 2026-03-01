FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Dublin

RUN apt-get update && apt-get install -y \
    ntpsec \
    ntpsec-ntpdate \
    vim \
    nano \
    cron \
    iputils-ping \
    traceroute \
    curl \
    tzdata \
    && rm -rf /var/lib/apt/lists/*

ARG USERNAME=oisin_mclaughlin
RUN useradd -m -s /bin/bash ${USERNAME}

RUN mkdir -p /home/${USERNAME}/ntp-logs && \
    chown -R ${USERNAME}:${USERNAME} /home/${USERNAME}

COPY ntp.conf /etc/ntpsec/ntp.conf

# Create the collection script
RUN echo '#!/bin/bash' > /home/${USERNAME}/collect_ntp.sh && \
    echo 'echo "=== $(date -u '"'"'+%Y-%m-%d %H:%M:%S UTC'"'"') ===" >> /home/oisin_mclaughlin/ntp-logs/ntplog.txt' >> /home/${USERNAME}/collect_ntp.sh && \
    echo 'ntpq -p >> /home/oisin_mclaughlin/ntp-logs/ntplog.txt' >> /home/${USERNAME}/collect_ntp.sh && \
    echo 'echo "" >> /home/oisin_mclaughlin/ntp-logs/ntplog.txt' >> /home/${USERNAME}/collect_ntp.sh && \
    chmod +x /home/${USERNAME}/collect_ntp.sh

# Set up cron job
RUN echo "*/20 * * * * /home/oisin_mclaughlin/collect_ntp.sh" | crontab -

WORKDIR /root

# Start both cron and keep container running
CMD service cron start && ntpd -n