#!/bin/sh
#Author: Sumit Kumar Agrawal
#quickly create a language database file
# first file should have values such as first line contains all english value strings separated by comma and second line contains all second language string separated by comma. (This script doesn't do any validation. so better be sure before ... ;-) )

#"abc","cde"
#"uvw","pqrs"

if [ "$#" != "2" ]; then
	echo "Usage: convert.sh <googlefile> <outputfile>" 
	exit 0
fi

i=0
english=
other=

while read line
do
	if [ "$i" == "0" ]; then
		english=$line
	else
		other=$line
	fi
	i=`expr $i + 1`
done < $1

if [ -z "$2" ]; then
	rm -rf $2
fi

touch $2
echo "" >> $2

IFS=','
i=1
echo $other

for word in $english
do
	oword=`echo $other | cut -d" " -f${i} | cut -d"\"" -f2`
	eword=`echo $word | cut -d"\"" -f2`
	echo "ENG $eword" >> $2
	echo "TRANS $oword" >> $2
	echo "DONE" >> $2
	i=`expr $i + 1`
done
