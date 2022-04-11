#! /bin/bash

sed -n '/include::{exampledir}\(.*\)\[\]/{s%%modules/ROOT/examples/\1%p}' \
    modules/ROOT/pages/documentation.adoc | paste -sd " " -
