# PunchedTapeScanner
An OpenCV program that extracts a data dump from a scanned image of a puched tape.

# Compile it

>  mkdir build  
>  cd build  
>  cmake ..  
>  make  

# Use it

There's an example of a scanned punched tape in this repository. The file is called __example/fita_absoluta.2a.0006.96.scan03.png__

This tape contains a tiny hello world program that prints the text string "PATO FEIO" (it means 'ugly duck' in Brazilian portuguese) in the DECWRITER printer, attached to the Patinho Feio computer.

It was the first computer designed in Brazil (in the early 70s) and it has a new driver for emulating it in MAME.

To extract a ROM image of the tape, run the program like this:

>  ./PunchedTapeScanner ../example/fita_absoluta.2a.0006.96.scan03.png

The result will be a file called output.rom that you can examine with:

>  hexdump -C output.rom | less
