=== compile at fly ===
cd bk-player
macro11 -o mon.o -l mon.lst bkemu-mon.asm
linkbk -v -o mon.bin mon.o
./dump.sh

=== compile at void ===
cd ~/src/BK8266
vim soft/EmuAPP/ROM/rom.h

cd soft/EmuAPP/ROM/
gcc -o mkrom mkrom.c
./mkrom
./build.sh

# change 53
scp builds/53/0x00000.bin c720w:tmp

=== write ESP flash  at c720w ===
#esptool.py -p /dev/ttyUSB1 -b 115200 write_flash --flash_size detect 0x0000 ~/tmp/0x00000.bin
#root
esptool.py -b 115200 write_flash --flash_mode dio --flash_size detect 0x0000 /home/rabbit/tmp/0x00000.bin

====================== firmware ================

== at fly ==
gmake
scp .out/cdrom-player.bin dfbrowser@fly:var/

== at c720w ==
minichlink -w ~/tmp/cdrom-player.bin flash -b -T
