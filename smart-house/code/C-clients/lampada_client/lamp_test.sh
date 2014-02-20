#!/bin/bash
clear
ADDR=$1
echo "Typed address: $ADDR"

ping6 $ADDR -c 3
echo ""
sleep 2

./status_lamp
echo ""
sleep 2

./turn_on
./status_lamp
echo ""
sleep 5

./turn_off
./status_lamp
echo "Test has finished"
