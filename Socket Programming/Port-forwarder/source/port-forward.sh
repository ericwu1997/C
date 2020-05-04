#! /bin/bash
IFS=$'\n'
set -f
for i in $(cat < "setup.config"); do
    mapfile -t IP < <(echo $i | awk '{gsub(":", " ",$0); print}' | awk '{print $1 " "$3 " "$4}')
    echo $IP
    arr=($IP)
    eval "./server ${arr[0]} ${arr[1]} ${arr[2]} &" 
    echo "kill -9 '$!' | echo 'service stopped'" >> stop-script.sh
done
echo "echo '' > stop-script.sh " >> stop-script.sh