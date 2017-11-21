# IDE_trial
Project to talk to IDE interface using Raspberry-Pi GPIO pins in SMI (Secondary Memory Interface bus) mode

(Code still to come) 
I recently found the remains of a project I started three years ago on my hard drive.
The idea was to use the Raspberr-Pi GPIO pins in SMI mode (which stands for Secondary Memory Interface) to talk to
an IDE interface. Although IDE is old there are still IDE to SATA adapters so it might be used to talk to SATA as well.

I did get some of it working. I managed to use PIO mode to read and write sectors on an old Seagate IDE drive.
(40 pin interface, NOT the 80 pin) With a bit of C-code optimization I got to about 44Mbyte/sec.
However I never managed to get the disk run DMA cycles.
As far as I could tell my command was OK, the interface signals where OK but the disk would just not respond.
I send a support request to Seagate but of course never got a reply.
I also found the wave forms in the ATAPI standard horrific. Who ever made those has never, ever seen a real datasheet.
Maybe I did interpret them wrong!

In this repository you will find the state of the project as I left it.
It was NOT written for publication, nor have I done any post cleaning.
(So you can see how my code looks like if I write for myself :-)

