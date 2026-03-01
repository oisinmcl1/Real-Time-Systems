#!/bin/bash

echo "Starting NTP Assignment Container..."

if [ "$(docker ps -aq -f name=ntp-assignment)" ]; then
    echo "Container exists, starting it..."
    docker start ntp-assignment
else
    echo "Creating new container..."
    docker run -dit \
      --name ntp-assignment \
      --privileged \
      -v "$(pwd)/ntp-logs:/home/oisin_mclaughlin/ntp-logs" \
      ntp-assignment \
      /bin/bash -c "service cron start && service ntpsec start && tail -f /dev/null"
fi

echo "Container started!"
echo ""
echo "To enter the container, run: ./enter.sh"
echo "To check NTP status, run: ./status.sh"