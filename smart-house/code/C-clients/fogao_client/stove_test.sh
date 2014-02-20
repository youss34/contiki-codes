#!/bin/bash
clear
ADDR=$1
echo "Typed address: $ADDR"

ping6 $ADDR -c 3
echo ""
sleep 2

./status_fogao
echo ""
sleep 2

./temperature
echo "Test has finished."

