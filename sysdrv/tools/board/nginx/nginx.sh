#!/bin/sh

if [ ! -d /tmp/nginx ]; then
    mkdir /tmp/nginx
fi

alias nginx='nginx -p /root/web'