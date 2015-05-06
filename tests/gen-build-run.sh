#!/bin/bash

../bin/cppXb -ns tst -r -xs xsd

cd build

make

cd ..

./bin/cppXbTest

