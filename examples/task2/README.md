## Examples
This examples are my efforts to understand how the board works.

Here you can find two examples, compile eachone to a board to see them comunicate.

To compile them you can use this command:
```
make distclean
make -j8 TARGET=zoul BOARD=remote-revb nullnet-broadcast.upload
make TARGET=zoul BOARD=remote-revb login
```
Don't forget to change permissions to the USB interface and select the appropriate number of cores for the Makefile.
Don't forget to add
```
MAKE_MAC = MAKE_MAC_TSCH
```
To the Makefile
Dont't forget to set the right coordinator addres.

More info at [this guide](https://github.com/Zolertia/Resources/wiki/Getting-Started-with-Zolertia-products).