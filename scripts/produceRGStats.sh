#!/bin/bash

filename=$1

if [ "X${filename}" = "X" ]
then
    echo "usage: `basename $0` <rg4 file>"
    exit 1
fi

if [ ! -f $filename ]
then
    echo "\"${filename}\" file doesn't exist"
    exit 1
fi

targetFilename=`echo ${filename}|sed -e "s/\.rg$/.xml/g"`

#echo "output filename = ${targetFilename}"

if [ ${targetFilename} = ${filename} ]
then
    echo "${filename} doesn't appear to be a valid RG file"
    exit 1
fi

tempFile=".tmpFile"

# uncompress and rename
/bin/gunzip -c ${filename} > ${tempFile}

# add the XSL link
echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" > ${targetFilename}
#echo "<?xml-stylesheet type=\"text/xsl\" href=\"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/*checkout*/rosegarden/gui/testfiles/rg-stats.xsl?rev=HEAD&content-type=text/plain\"?>" >> ${targetFilename}
echo "<?xml-stylesheet type=\"text/xsl\" href=\"rg-stats.xsl\"?>" >> ${targetFilename}

count=`wc -l ${tempFile} |awk '{ print $1}'`
count=`echo "${count} - 2"|bc`
tail -${count} ${tempFile} >> ${targetFilename}

