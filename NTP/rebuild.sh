#!/bin/bash

echo "Rebuilding NTP Assignment..."

# Stop and remove old container
docker stop ntp-assignment 2>/dev/null
docker rm ntp-assignment 2>/dev/null

# Rebuild image
docker build -t ntp-assignment .

# Start new container
./start.sh