## Parallelised Benchmarks

This folder contains benchmark variants that support uxn4-on-gpu's parallelisation API. 

### Stencil
[stencil.tal](stencil.tal) - The original stencil benchmark. 

[stencil1.tal](stencil1.tal) - On top of the original version, fixed the addressing to the p array so results are treated as short instead of byte. Instead of printing loop iterators, it prints final computation result instead.

[stencil2.tal](stencil2.tal) - Simulates the behaviour of stencil3.tal with a single invocation, without actually invoking the parallelisation device. Can run on an Uxn implementation without the paralellisation device, and should produce the same result as stencil3.tal.

[stencil3.tal](stencil3.tal) - Fully parallelised version of stencil.tal. It uses double buffering for computation result at t and t-1, and uses a invocation id addressable array for invocation local variables.

### Mandelbrot
[mandelbrot.tal](mandelbrot.tal) - The original mandelbrot benchmark.

[mandelbrot1.tal](mandelbrot1.tal) - A row-level parallelised version of mandelbrot.tal. mandelbrot.tal with default setting has 144 rows, so the max number of non-idle invocation is 144 (0x90).

### Bunnymark

[bunnymark.tal](bunnymark.tal) - The original bunnymark benchmark.

[bunnymark1.tal](bunnymark1.tal) - bunnymark.tal with mouse input disabled, and populate a constant number of bunnies at the program start instead.

[bunnymark2.tal](bunnymark2.tal) - bunnymark.tal with mout input disabled, and parallelisation enabled. The parallelisation is on per-bunny level.


### Tri(angle)

[tri.tal](tri.tal) - The original tri benchmark.

[tri1.tal](tri1.tal) - tri.tal with row-of-pixel-inside-triangle level parallelisation. Currently relies on full stack copy to produce the correct result. 

[tri2.tal](tri2.tal) - tri.tal with per-triangle paralleisation. Incomplete and under construction.

