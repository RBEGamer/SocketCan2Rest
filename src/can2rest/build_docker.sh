#!/bin/bash

docker build -t osmrican2rest:latest .
docker run -it --rm osmrican2rest:latest