#!/bin/bash

echo "NTP Status:"
echo "=============="
docker exec ntp-assignment service ntpsec status
echo ""
echo "NTP Peers:"
echo "============="
docker exec ntp-assignment ntpq -p