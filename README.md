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
$ git clone https://github.com/saitoha/NeHe-SIXEL
$ cd NeHe-SIXEL/lesson11
$ ./configure
$ make
$ ./lesson11
```

## License

Unknown...

This code seems to be originally written by Bosco (bosco4@home.com).
Ported to Linux by Richard Campbell (ulmont@bellsouth.net).
Hayaki Saito (saitoha@me.com) modified it for SIXEL devices.

Original Credits:

```
Contact:

If you have problems, comments, or have useful hints,
email me at ulmont@bellsouth.net.

Credits:
Jeff Molofee (nehe@home.com) for writing the tutorials.
Alfred (alfred@mazuma.net.au) for various fixes and improvements.
Lakmal Gunasekara (gunasekara@prostep.de) for a Solaris/SPARC
        and SGI/IRIX port, which has improved the portability
        of the code base.  Also for tutorial 8, which I only
        tweaked slightly (basically uncommented what had been
        commented out for Solaris).

-Richard Campbell.
```

NeHe's Readme.txt

```
==========================================================================
        OpenGL Lesson 11:  Waving Texture Map (Modifying The Mesh)
==========================================================================

  Authors Name: Bosco

  Disclaimer:

  This program may crash your system or run poorly depending on your
  hardware.  The program and code contained in this archive was scanned
  for virii and has passed all test before it was put online.  If you
  use this code in project of your own, send a shout out to the author!

==========================================================================
                        NeHe Productions 1997-2004
==========================================================================
```
