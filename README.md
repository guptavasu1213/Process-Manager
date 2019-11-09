# Process Manager

## Table of contents
* [Overview](#overview)
* [Prerequisites](#prerequisites)
* [Getting Started](#getting-started)
* [Building](#building)
* [Status](#status)
* [Authors](#authors)

## Overview

This program is process manager which creates new processes and keeps track of them. The program performs a check on the running processes every 5 seconds and displays the output accordingly. The program terminates when there are no active processes running.

The processes to be created are specified on each line in the .config file with a maximum time (in seconds) they are allowed to run for.

This is an similar implementation of utilities like **[Cron](https://en.wikipedia.org/wiki/Cron)**, which is a time-based job scheduler.

## Prerequisites

You need to have GCC to compile this file.
This program works on the gcc version (SUSE Linux) 4.8.5

To check if GCC is installed, run the following command on the Linux/UNIX terminal:
```
gcc --version
```

## Getting Started
* Clone/download this repository in a local computer
* Create an executable by running the following target from the makefile:
```
make all
```

## Building
### Usage
* There are two know flags which can be passed when running the program
* -f is specified if the output is to be displayed to stdout. If not specified, the output of the parent process is redirected to a log file named "macD.log". 
* -o is specified if the output of the child processes ran using .conf file is to be redirected to a file passed as an argument. If the flag is not specified, then the output goes to /dev/null. 
```
./macD [-o outputFileName] [-f]
```

### Example
* Specifying the -o flag to redirect the output of child processes to a file:
```
./macD -o output.txt
``` 
* Specifying the -f flag to redirect the output of the parent process to stdout:
```
./macD -f
```
* Specifying the -f and -o flag to redirect the output of child processes to a file and redirect the output of the parent process to stdout.
```
./macD -f -o output.txt
``` 

## Status
The project is finished.

## Authors
* Vasu Gupta 
