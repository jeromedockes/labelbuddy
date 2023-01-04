#! /bin/bash

apt-get update
apt-get install -y cmake file lsb-release

codename=$(lsb_release -cs)

case $codename in

    buster|stretch)
        apt-get install -y qt5-default
        apt-get install -y build-essential
        ;;

    *)
        apt-get install -y cmake
        apt-get install -y g++
        apt-get install -y qtbase5-dev
        ;;
esac
