#for f in *.ogg;
#do
#echo "Converting $f";
#g=`$f .ogg`;
#ffmpeg -i $f -acodec libvorbis -ar 32000 $g .ogg || echo FAILED;
#done

for i in *.ogg;
  do name=`echo $i | cut -d'.' -f1`;
  echo $name;
  ffmpeg -i $i -acodec libvorbis -ar 32000 conv/$name.ogg;
done
