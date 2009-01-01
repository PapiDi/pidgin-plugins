#!/bin/sh
#Author: Sumit Kumar Agrawal
#Helper script to quickly create entries for unicode values

if [ "$#" != "2" ]; then
	echo "Usage: create_unicode_hash.sh <start-number> <end-number>" 
	exit 0
fi
number=$1
endnumber=$2
while true 
do
	hexnumber=`echo "obase=16;$number"| bc`
	if [ ""`expr length $hexnumber` == "2" ]; then
		hexnumber="00$hexnumber"
	fi
	if [ ""`expr length $hexnumber` == "3" ]; then
		hexnumber="0$hexnumber"
	fi
	value=`/usr/bin/printf "\u$hexnumber"`
	# remove leading white spaces
	value=${value## }
	# remove trailing white spaces
	value=${value%% }
	if [ ""$value != "" ]; then
		echo "g_hash_table_insert(unicode_hash, \"&#$number\", \"$value\");" >> tmp.txt
	fi
	number=`expr $number + 1`
	if [ "$number" == "$endnumber" ]; then
		break;
	fi
done 

value=`/usr/bin/printf "\u0B86"`
echo $value
