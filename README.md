# PunchedTapeScanner
An OpenCV program that extracts a data dump from a scanned image of an 8-channel punched tape.

# Compile it

>  mkdir build  
>  cd build  
>  cmake ..  
>  make  

# Use it

For a typical roll of paper tape with the start of the data on the outside of the roll, unroll the initial part of the tape and place the outer side of the tape against the glass on a flatbed scanner. (If the tape is longer than 256 bytes, you'll need to do multiple scans and extractions, and manually stitch the results together.)

For best results:
- Place a dark background on top of the tape before scanning.
- Scan in color at 300 dpi.
- Crop the image to 1/4 inch on either side of the tape.

The scanned image might look something like this:

	            -----------
	           / * *      /
	          /   *      /
	         /   *      /
	        /........../
	start  <     **   <   end
	        \      *   \
	         \     **   \
	          \     *    \
	           \ * *      \
	            -----------

To extract the data from the image:

>  ./PunchedTapeScanner img.png output.rom 1

If you accidentally placed the inner side of the tape against the scanner glass, use:

>  ./PunchedTapeScanner img.png output.rom 0

By default, PunchedTapeScanner extracts 8-bit binary data. If your tape is 7-bit ASCII text with a parity bit, change this line:

	unsigned char BIT_MASK = 0xFF;

to:

	unsigned char BIT_MASK = 0x7F;

before compiling.

# Example

There's an example of a scanned punched tape in this repository. The file is called __example/fita_absoluta.2a.0006.96.scan03.png__

This tape contains a tiny hello world program that prints the text string "PATO FEIO" (it means 'ugly duck' in Brazilian portuguese) in the DECWRITER printer, attached to the Patinho Feio computer.

It was the first computer designed in Brazil (in the early 70s) and it has a new driver for emulating it in MAME.

To extract a ROM image of the tape, run the program like this:

>  ./PunchedTapeScanner ../example/fita_absoluta.2a.0006.96.scan03.png output.rom 0

The result will be a file called output.rom that you can examine with:

>  hexdump -C output.rom | less
