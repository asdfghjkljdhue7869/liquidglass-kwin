#!/bin/bash
P=$HOME/projects/liquidglass-kwin
if [ -f $P/nested.sh.bak3 ]; then cp $P/nested.sh.bak3 $P/nested.sh; fi
rm -rf $P/stage $P/build $P/testclient /tmp/lgtest
echo "rolled back 4b"
