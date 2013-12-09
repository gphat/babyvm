# Baby's First Garbage Collector

This is an implementation of a simple mark & sweep garbage collector as
described at Bob Nystrom's blog post
[Baby's First Garbage Collector](http://journal.stuffwithstuff.com/2013/12/08/babys-first-garbage-collector/).

## Description

Bob's code was easy to follow, but I was interested in seeing it work for myself. I though it
would be useful to create a barebones projects for other interested in toying with a simple
garbage collector.

I also took the liberty of peppering some `printf`s around to report the status of various things.

It's also been many years since I wrote any C and this was a nice reminder of how to do it.

## Compiling

Requires make and gcc.  Compile with `make` and run `make clean` to destroy the evidence.

## Running

After compiling simply execute the `babyvm` binary and enjoy!
