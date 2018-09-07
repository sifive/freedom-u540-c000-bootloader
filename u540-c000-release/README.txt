This is a build from the original tapeout that went
into the boot ROM on the HighFive Unleashed. 

The challenge is to produce a zsbl.bin from the code
in this repository that is as close to the bootrom.bin
as you can get.

shasum -a 256 bootrom.bin 
f9353bab5e826e937783c9ace7411375af9733c454d3d5f9f365509f2f7a4cdb  bootrom.bin

Please note that the original rom and fsbl did NOT have
git ID strings, and the ordering of some output in main.c
is a little different.
