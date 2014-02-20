#!/bin/bash
clear
ADDR=$1
echo "Typed address: $ADDR"

ping6 $ADDR -c 3
echo ""
sleep 2

./status_term
echo ""
sleep 2

./turn_on
./status_term
echo ""
sleep 5

./temperature
echo ""
sleep 5

./status_term
echo ""
sleep 5

./turn_off
./status_term
echo "Test has finished"
