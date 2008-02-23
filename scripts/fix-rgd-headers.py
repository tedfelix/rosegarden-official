#!/usr/bin/python
#
# From: "Anders Dahnielson" <anders@dahnielson.com>
#
# PURPOSE: rewrite .rgd files with header that is more universally recognized as
# belonging to us
#

import sys, gzip                                                          
                                                                          
inf = open(sys.argv[1], 'r')                                              
zipf = gzip.GzipFile(mode='r', fileobj=inf)                               
content = zipf.read()                                                     
zipf.close()                                                              
inf.close()                                                               
                                                                          
outf = open(sys.argv[1], 'w')                                             
zipf = gzip.GzipFile(mode='w', filename='audio/x-rosegarden-device', fileobj=outf)                                                             
zipf.filename = "audio/x-rosegarden-device"                               
zipf.write(content)                                                       
zipf.close()                                                              
outf.close()
