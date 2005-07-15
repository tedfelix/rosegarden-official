files=`find . -type f -print`
echo "cd rosegardenmusic.com/website" > upload_script_$$
for f in $files; do
    if echo $f | grep CVS >/dev/null; then continue; fi
    if echo $f | grep '~' >/dev/null; then continue; fi
    if echo $f | grep 'upload' >/dev/null; then continue; fi
    f=${f#./}
    path=${f%/*}
    if [ "$path" = "$f" ]; then path=""; fi
    name=${f##*/}
    up=`echo $path | sed 's/[^/.][^/.]*/../g'`
    [ -n "$path" ] && echo "cd $path"
    [ -n "$path" ] && echo "lcd $path"
    echo "put $name"
    [ -n "$up" ] && echo "cd $up"
    [ -n "$up" ] && echo "lcd $up"
done >> upload_script_$$
sftp -b upload_script_$$ cannam@cannam.ukfsn.org
