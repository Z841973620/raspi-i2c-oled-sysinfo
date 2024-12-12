#!/bin/bash
cd ./WiringPi && ./build && cd ..
gcc -o sysinfo sysinfo.c ssd1306_i2c.c -lwiringPi
chmod 777 sysinfo && mv sysinfo /usr/local/bin/
echo -e '[Unit]\nDescription=sysinfo\nAfter=network.target\n\n[Service]\nType=forking\nExecStart=/bin/bash -c "/usr/local/bin/sysinfo &"\nExecStop=/bin/kill -SIGINT $MAINPID\n\n[Install]\nWantedBy=multi-user.target' > /usr/lib/systemd/system/sysinfo.service
systemctl daemon-reload && systemctl enable sysinfo && systemctl start sysinfo
