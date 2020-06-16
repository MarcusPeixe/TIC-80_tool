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

# Cover image import

The great thing about this particular part of the program is: it doesn't care
about the gif contents, if it is the right size, whether it is animated or not...
This means that you might want to take extra care and make sure your gif isn't
corrupted or anything (although if it is, probably nothing bad will happen), but
it does open up the possibilities for very interesting covers, those of which would
be impossible to get naturally (without any external tools just like this one).
	
The key here is, TIC already accepts gifs with full colour, and if you export an
animated one, the full animation will be visible on the website, but only the last
frame will be displayed on SURF mode (at least it doesn't break). A cover with the
wrong dimensions, animated or not, will be broken in SURF mode, but will still
show up on the website though.
