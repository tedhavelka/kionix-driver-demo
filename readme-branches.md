

2021-10-25 MON
New branch 'cli-dev-work-002' based on 'iis2dh-driver-development-and-tests' with hash 93681bd120747041085459b7c1bc89179c5a568d.  Some key features, very simple, working in this branch:

*  CLI uses UART2 on Sparkfun Thing Plus nRF9160 development board.  This port distinct from UART-to-USB or similar on board peripheral that's defined in device tree as UART0.
*  Simple command string capture work.
*  <ESC> aborts latest command.
*  Prompt generation working, uses Zephyr in-tree uart_poll_out() API function.  (Must research whether this an appropriate use of resources in embedded system.)
  



