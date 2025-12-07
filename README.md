# Sum System

This program demonstrates a threaded summation pipeline using POSIX threads and synchronized shared resources. One hundred producer threads generate random integers, a dispatcher coordinates pairing values for computation, and summator threads iteratively reduce the buffer until a single result remains.

## Architecture

- **Producers (100 threads):** Each sleeps 1–7 seconds, generates a random integer (1–100), pushes it to the shared buffer, logs the action, and exits.
- **Bounded buffer:** Circular queue with mutex and two condition variables enabling blocking put/get operations. Capacity defaults to 128 to accommodate all producers.
- **Dispatcher:** Waits without busy looping. When at least two items are available, it removes a pair and spawns a summator. It tracks active summators and terminates only when producers are done, no summators remain, and the buffer holds one final element.
- **Summators:** Sleep 3–6 seconds, compute the sum of their pair, push the result back to the buffer, log progress, and decrement the active-summator counter.
- **Logging:** Thread-safe printing with relative timestamps to stdout for reproducible, readable traces.

## Building

```sh
make
```

This compiles the `sum_system` executable using `gcc` with POSIX thread support.

## Running

```sh
./sum_system
```

On each execution the random seeds produce different delays and values, but the protocol remains deterministic: threads log start/finish messages, and the dispatcher reports the final value once only one number remains in the buffer.

## Project Structure

- `main.c` — Initializes shared state, launches producers and dispatcher, joins threads, and prints completion messages.
- `buffer.c` / `buffer.h` — Bounded queue implementation with blocking put/get operations using mutexes and condition variables.
- `workers.c` / `workers.h` — Thread entry points and shared counters for producers, dispatcher, and summators.
- `log.c` / `log.h` — Thread-safe logging utilities with timestamped output.
- `Makefile` — Build targets for compiling (`make`) and cleanup (`make clean`).

## Notes

- The program avoids data races via disciplined mutex/condvar usage; no active waiting is used.
- Randomness is seeded in `main` with `srand(time(NULL))`; threads rely on `rand_r` for thread-safe generation.
