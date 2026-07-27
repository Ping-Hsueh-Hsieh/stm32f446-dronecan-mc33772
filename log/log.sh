stty -F /dev/ttyACM1 raw -echo && cat /dev/ttyACM1 | tee log.csv
