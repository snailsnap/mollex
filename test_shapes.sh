#!/bin/bash

buildpath='build'
datapath='data/gbif.naturkundemuseum-berlin.de/CDV2018/Mollusken/Arten' 
find "${datapath}" -name 'ZMB_Mol_*.jpg' -print -exec "${buildpath}/src/mollex" {} \;
