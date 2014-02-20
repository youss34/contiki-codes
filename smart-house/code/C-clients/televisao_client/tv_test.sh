#!/bin/bash
clear
ADDR=$1
echo "Typed address: $ADDR"

ping6 $ADDR -c 3
echo ""
sleep 2

./status_tv
echo ""
sleep 2

./turn_on
./status_tv
echo ""
sleep 5

./volume
./channel
echo ""
sleep 5

./status_tv
echo ""
sleep 5

./turn_off
./status_tv
echo "Test has finished"
