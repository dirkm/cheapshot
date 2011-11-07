#!/bin/bash

echo "make sure CPU is at maximum speed!!!"

make -s test_report > etc/test_report-$(exec date "+%Y%m%d-%H%M%S")
