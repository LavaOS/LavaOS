# LavaOS
LavaOS it's an free, open source unix-like operating system, forked of MinOS

## Build
get source code (On linux/bsd/mac):
```sh
git clone https://github.com/jrifuoue/LavaOS.git
cd LavaOS
```
(on windows, you can download the source code .zip file and the uncompress it)
build the compiler (linux):
```sh
gcc nob.c -o nob
```
build the compiler (mac/bsd):
```sh
clang nob.c -o nob
```
build the compiler (windows | you need visual studio with c/c++ support):
```sh
cl nob.c
```
build system and run(linux/mac/bsd):
```sh
./nob bruh
```
build system and run(windows):
```sh
nob bruh
```


## WARNING : On the first time, the nob.c will compile a huge custom gcc, make sure you have enough ram and swapfile :)

## Contributing

I'm open to contributions!

I love when the community helps in making something good, but I have to point out a few things.

Please don't make contributions that:
- Make something intentionally more obscure
- Add a huge dependency without much reasoning
- Implement or change an enormous part of the code and its structure (unless justified)
