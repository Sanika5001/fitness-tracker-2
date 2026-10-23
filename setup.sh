#!/usr/bin/env bash
set -e
mkdir -p backend/third_party
echo "Downloading httplib.h..."
curl -sSL -o backend/third_party/httplib.h https://cdn.jsdelivr.net/gh/yhirose/cpp-httplib@v0.18.3/httplib.h
echo "Downloading json.hpp..."
curl -sSL -o backend/third_party/json.hpp https://cdn.jsdelivr.net/gh/nlohmann/json@v3.11.3/single_include/nlohmann/json.hpp
echo "Downloading picosha2.h..."
curl -sSL -o backend/third_party/picosha2.h https://cdn.jsdelivr.net/gh/okdshin/PicoSHA2@master/picosha2.h
echo "Third party dependencies downloaded successfully."
