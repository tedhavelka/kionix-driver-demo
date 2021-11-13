

2021-10-25 MON
New branch based on 'iis2dh-driver-development-and-tests' with hash 93681bd120747041085459b7c1bc89179c5a568d.

Command Line Interface work in branches:

*  cli-dev-work-002   . . . basic line input, and simple sensible echoing over arbitrary UART.
*  cli-dev-work-003   . . . line parsing into first token and remaining, test of setting and getting static global vars between Zephyr threads
*  cli-dev-work-004   . . . array of command handlers (structures with function ptrs) and basic scoreboard module implemented

At point of CLI dev work 004 there is a basic framework in place for adding new CLI commands.  There are basically three places to add supporting expressions and definitions for a given, new command.  These places are (1) a function prototypes area, (2) a command handler entry in array of structures, and (3) the routine definition to handle the new command.


