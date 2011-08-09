#!/bin/sh
script="$1"
grep_word="$2"
grep_param="$3"

if [ "$grep_param" != "" ]; then
    ./waf --run "$script" 2>&1 | grep "$grep_param" "$grep_word" 2>&1
else
  ./waf --run "$script" 2>&1 | grep "$grep_word" 2>&1
fi

echo -e "\a"
sleep 1
echo -e "\a"