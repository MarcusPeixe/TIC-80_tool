# TIC-80 tool

This tool can be used to extract and insert code or the cover gif from and to TIC-80 cartridges. It works best on a Linux terminal, and you can put the compiled binary on the `/bin` folder or something, if you want to.

Code for handling .tic files (see .tic format specification
here: https://github.com/nesbox/TIC-80/wiki/tic-File-Format)

You can inject external .lua files, as well as .gif files
for the cover. You can also export the current ones present
inside the cartridge.

If external files are greater than 65536 bytes (65KB, 2^16B),
then the program will simply reject them (impossible to insert
them into the cart, due to 16-bit limitations).

Certain .gif files may not work well when in SURF mode, but
they do work for carts shared in https://tic.computer/play
