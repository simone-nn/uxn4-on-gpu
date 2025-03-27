# Bundled programs
These are some bundled programs taken from various sources.
Programs that were not created by me or contain piece of code from others
will credit the source inside the `.tal` file.
Programs are grouped by how well they function on this implementation of the VM.
All programs are functional on the [100R](https://100r.co/site/uxn.html) runtime.  

- `.tal` uxntal source code file
- `.rom` compiled uxn assembly ready to be loaded up in the VM

## Functional
- `auto` tests the Screen/auto port.
- `corners` displays sprite approximately in the windows corners.
- `dvd` the classic bouncing dvd program
- `image` displays some art
- `pixel` terrible first uxn program I wrote; it displays some coloured pixels
- `snake` the classic snake game on uxn
- `spacing` type in a string and it will print it out spaced out
- `unit_test` tests for various instructions that were tricky to implement

## Semi Functional
- `bunnymark` benchmarking program; is very glitchy
- `fib` calculates the fibonacci series. Gives weird results even on the official runtime.
- `mouse` tests mouse support. Does not render properly for some reason
- `xo` tic tac toe. Board displays but nothing else seems to work

## Not Function
- `cube3d` should render a 3D cube
