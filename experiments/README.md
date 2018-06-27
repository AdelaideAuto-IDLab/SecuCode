# SecuCode/experiments

This directory contains the experiment setup and firmware to perform the key derivation and hash as well as the end-to-end update.
Overview

## Overview

* `/End-to-end`: Contains the result for the case study "Plaster embedded CRFID device".
* `/Hash`: Contains the materials for the hash function evaluation.
  - `/Code`: Special firmware to perform hash on cold start-up.
  - `/Data`: Results
  - `/Tools`: Matlab script to interprept the results.
* `/KeyDerivation`:Contains the materials for the key derivation evaluation.
  - `/Code`: Special firmware to perform key derivation on cold start-up.
  - `/Data`: Results
  - `/Tools`: Matlab script to interprept the results.

## Setup the experiment

For Key derivation experiment:
![](https://github.com/AdelaideAuto-IDLab/SecuCode/blob/master/experiments/Hash/HKsetup.PNG)

For end-to-end firmware update experiment:
![](https://github.com/AdelaideAuto-IDLab/SecuCode/blob/master/experiments/End-to-end/uPUF-Plaster2.jpg)
