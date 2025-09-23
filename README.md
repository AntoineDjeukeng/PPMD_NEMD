# Producer/Consumer Demo (C11 + pthreads)

A minimal, clear example of a single producer and multiple consumers sharing a small ring of slots using C11 atomics and POSIX threads.

- Producer fills a slot and publishes it for consumers.
- Each consumer processes the most recent published frame, then decrements a reference counter.
- When the last consumer finishes with a slot, the producer can reuse it.

## Build

- With Makefile (recommended):

```
make
```

- Direct `gcc` example:

```
gcc -O2 -std=c11 -Wall -Wextra -pedantic -pthread -latomic main.c -o main
```

Notes:
- `-std=c11` for `<stdatomic.h>`; `-pthread` for POSIX threads.
- `-latomic` is included for portability on some platforms; harmless where not needed.

## Run

- Run directly:

```
./main
```

- Stop with `Ctrl+C` (the demo runs indefinitely).

- Quick preview (1 second) to see sample output:

```
timeout 1s ./main
```

## Tuning

Edit `main.c` configuration constants:
- `THREAD_COUNT`: 1 producer + `THREAD_COUNT-1` consumers (default 8)
- `SLOTS`: number of ring slots (default 4)
- `MAX_FRAME_F64`: demo frame capacity (unused in practice here beyond bounds)

## How it works (brief)

- Producer waits for a free slot (refs==0), fills it, sets `refs = NUM_CONSUMERS`, and publishes the index and generation under a mutex + condition variable.
- Consumers wait for a new generation, read the published slot without locks, then `fetch_sub` the `refs`. The last consumer signals the producer that reuse is possible.

## Clean

```
make clean
```

