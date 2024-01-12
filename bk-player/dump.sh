#!/bin/sh
od mon.bi-PMAIN.v|cut -w -f2-|sed -e 's/  /, 0/g' -e 's/\t/, 0/g' -e's/^/0/' -e 's/$/,/'
