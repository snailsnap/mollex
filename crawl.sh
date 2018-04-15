#!/bin/bash

wd="$(pwd)"
mkdir data && cd data
wget -v -r -l1 --no-parent -A.jpg http://gbif.naturkundemuseum-berlin.de/CDV2018/Mollusken/Arten/
cd "$wd"
