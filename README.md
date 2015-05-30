NeHe-SIXEL
==========

[![nehe](https://raw.githubusercontent.com/saitoha/NeHe-SIXEL/data/data/nehe.png)](https://youtu.be/cn_qty-452s)

## Step 0. Edit .Xresources

```
$ cat $HOME/.Xresources
XTerm*decTerminalID: vt340
XTerm*sixelScrolling: true
XTerm*regisScreenSize: 1920x1080
XTerm*numColorRegisters: 256

$ xrdb $HOME/.Xresources  # reload
```

## Step 1. Build xterm with --enable-sixel-graphics option

```
$ wget ftp://invisible-island.net/xterm/xterm.tar.gz
$ tar xvzf xterm.tar.gz
$ cd xterm-*
$ ./configure --enable-sixel-graphics
$ make
$ ./xterm  # launch
```

## Step 2. Build and install libsixel

```
$ git clone https://github.com/saitoha/libsixel
$ cd libsixel
$ ./configure && make install
```

## Step 3. Build a lesson

```
$ cd ..
$ git clone https://github.com/saitoha/NeHe-SIXEL
$ cd NeHe-SIXEL/lesson11
$ ./configure
$ make
$ ./lesson11
```

