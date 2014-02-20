#!/bin/bash
clear
ADDR=$1
echo "Typed address: $ADDR"

ping6 $ADDR -c 3
echo ""
sleep 2

./status_radio
echo ""
sleep 2

./turn_on
./status_radio
echo ""
sleep 5

./volume
./station
echo ""
sleep 5

./status_radio
echo ""
sleep 5

./turn_off
./status_radio
echo "Test has finished"
