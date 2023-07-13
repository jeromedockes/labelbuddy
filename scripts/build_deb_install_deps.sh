#! /bin/bash

apt-get update
apt-get install -y cmake file lsb-release bzr bzr-builddeb dh-make

codename=$(lsb_release -cs)

if [[ $codename == buster ]]; then
        apt-get install -y qt5-default
        apt-get install -y build-essential
else
        apt-get install -y g++
        apt-get install -y qtbase5-dev
fi
