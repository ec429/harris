#!/usr/bin/awk
BEGIN {FS=":"}
/^[[:alnum:]_]+:/	{print $1;}
