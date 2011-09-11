#!/bin/bash

make -s test_report > etc/test_report-$(exec date "+%Y%m%d-%H%M%S")
