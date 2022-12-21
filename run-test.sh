#!/usr/bin/env bash

mkdir /tinydb/build/build/data
rm -rf /tinydb/build/build/data/*
cd /tinydb/build/build; ./tinydb &

sleep 1s

/usr/bin/mysql --host 127.0.0.1 --port 12306 -u root
