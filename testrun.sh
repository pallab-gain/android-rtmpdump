#! /bin/bash
# r = url of rtmp link
# a = name of the target app on server
# W = URL to player swf file, compute hash/size automatically
# p = Web URL of played programme
# y = Overrides the playpath parsed from rtmp url
# o = output file
./rtmpdump -r "rtmpe://mobs.jagobd.com/tlive" \
-a "tlive" \
-W "http://tv.jagobd.com/player/player.swf" \
-p "http://www.mcaster.tv/channel/somoynews." \
-y "mp4:sm.stream" \
-o bd-tv.flv
