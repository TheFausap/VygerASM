# VygerASM
a VM (and assembler) written using OTCC

This is a very special C language. Based on K&R syntax, using the Obfuscated Tiny C Compiler, created by F. Bellard (https://bellard.org/otcc/).
The compiler itself is a joy and merciless... each syntax error is a "segmentation fault" :)

This version of C is really close to machine level. For example, No fancy stuff like FILE structures... only low level open, read, write.

## Memory and CPU

````{verbatim}
/* instruction format (generic) */ 
/* (r<<8)|(ind<<7)|(reg<<4)|op  */

/* instruction format (i/o)     */
/* (r<<8)|(t<<4)|op             */
/* t is:                        */
/*       00 = console to IOA    */
/*       01 = console from IOA  */
/*       02 = disk from IOA     */
/*       03 = disk to IOA       */

/* opcode format (integer operations) */
/* opR[#|i]nnn                        */
/* where R is regno.                  */
/*       # is immediate value         */
/*       i is indirect value          */
/*       m is the value or address    */
````
