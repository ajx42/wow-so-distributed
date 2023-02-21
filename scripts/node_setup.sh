#!/bin/bash

# Prerequisite: 
# - set up ssh keys on GitHub to access wow-so-distributed repo
# - modify SERVER_ADDRESS environment variable to point to the server
export SERVER_ADDRESS=\"c220g1-030827.wisc.cloudlab.us:50051\"

# update gcc
yes '' | sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get -y update
sudo apt-get -y install gcc-9 g++-9
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 60 --slave /usr/bin/g++ g++ /usr/bin/g++-9

# update git
yes '' | sudo add-apt-repository ppa:git-core/ppa
sudo apt -y update
sudo apt -y install git

# install cmake
sudo apt update -y && \
sudo apt install -y software-properties-common lsb-release && \
sudo apt clean all
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
sudo apt update -y
sudo apt install -y kitware-archive-keyring
sudo rm /etc/apt/trusted.gpg.d/kitware.gpg
sudo apt update -y
sudo apt install -y cmake

# git clone libfuse
sudo apt-get install -y fuse libfuse-dev

# git clone and build wowfs
git clone --recurse-submodules git@github.com:ajain365/wow-so-distributed.git
cd wow-so-distributed
mkdir build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=.
cmake --build . --parallel 16
cmake --install .

# upgrade python
cd ~
yes '' | sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt-get -y update
sudo apt-get install -y python3.9
sudo update-alternatives --install /usr/bin/python python /usr/bin/python3.9 60
sudo apt install -y python3.9-distutils

# install pip
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
python get-pip.py

# install paramiko for consistency_test
python -m pip install paramiko