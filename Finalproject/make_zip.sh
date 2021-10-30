#!/bin/bash
cd dcash_wallet; make clean ; cd ..;
zip -r submission.zip * -x "*.git*" -x "*.DS_Store"
