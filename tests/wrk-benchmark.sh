#!/bin/bash

wrk -t12 -c400 -d3s --latency --script=./benchmark.lua http://localhost:8080/