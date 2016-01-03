## Device Drivers

This directory contains the files for the device drivers currently implemented:

- The test display: it defines displays (e.g. text windows)
- The keyboard: the keyboard driver is invoked when IRQ 1 is called. Note that all it does is that it just stores on the focus process buffer.
