#!/bin/sh

network_init()
{
        ethaddr1=`ifconfig -a | grep "eth.*HWaddr" | awk '{print $5}'`

        if [ -f /data/ethaddr.txt ]; then
                ethaddr2=`cat /data/ethaddr.txt`
                if [ $ethaddr1 == $ethaddr2 ]; then
                        echo "eth HWaddr cfg ok"
                else
                        ifconfig eth0 down
                        ifconfig eth0 hw ether $ethaddr2
                fi
        else
                echo $ethaddr1 > /data/ethaddr.txt
        fi
        ifconfig eth0 up && udhcpc -i eth0
}

network_init &