#! /bin/bash

set -e

apt-get update
apt-get install -y cmake file lsb-release bzr bzr-builddeb dh-make

codename=$(lsb_release -cs)

apt-get install -y g++
apt-get install -y qtbase5-dev
