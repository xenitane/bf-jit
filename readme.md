# bf-jit

> A simple jit compiler for BrainFuck with some additional commands

## Table Of Contents

-   [bf-jit](#bf-jit)
    -   [Table Of Contents](#table-of-contents)
    -   [Inspiration](#inspiration)
    -   [Instruction set](#instruction-set)
    -   [Building](#building)
    -   [Usage](#usage)
        -   [Options](#options)
        -   [Examples](#examples)
    -   [References](#references)

## Inspiration

Who doesn't want to go ahead and make a programming language and that comes with further challenges, like writing a compiler, lsp, and other tooling around it. So to get started I just wanted to write a compiler for the programming language that reached a pinnacle, BrainFuck. With it's minimal instruction set it was pretty easy to write a compiler for it in plain old `C` and `x86 assembly`

## Instruction set

-   **+** : Increases the value of the current byte by one, flows to `0x00` in case of overflow
-   **-** : Decreases the value of the current byte by one, flows to `0xff` in case of underflow
-   **>** : Moves the memory pointer to the right by one, if not at the end of memory
-   **<** : Moves the memory pointer to the left by one, if not at beginning of memory
-   **.** : Prints the value of the current byte to `stdout`
-   **,** : Reads into the current byte from `stdin`
-   **[** : If the value of the current byte is `0x00`, then the instructions jump to the instruction next to that of the corresponding `]`
-   **]** : If the value of the current byte is not `0x00`, then the instructions jump to the instruction next to that of the corresponding `[`

## Building

> You just need a working C compiler in your system to get started with using this compiler, preferable `gcc`.

> Run the below commands on you shell/command line to build after cloning this repository and opening it in the cli

**Steps**:

-   `$ <c-compiler> -o nob nob.c`
-   `$ ./nob`

And you now have a working jit compiler for BrainFuck on your system named `bf-jit`.

## Usage

To use this compiler you just need to run the following command

```bash
$ path-to-bf-jit [options] <input.bf>
```

> This does not produces a binary for the given BrainFuck code, but runs it as is.

### Options

This compiler offers you some options under which your program will run, which are:

-   `--not-jit`: This turns of the jit feature of the compiler which is on by default.
-   `-M=<MemorySize>`: This takes in the number of bytes you want the program memory to be, `10000` by default.

### Examples

```bash
$ bf-jit input.bf
$ bf-jit --no-jit input.bf
$ bf-jit -M=20480 input.bf
$ bf-jit --no-jit -M=15000 input.bf
```

> You can find sample programs [here](https://en.wikipedia.org/wiki/Brainfuck) and i have also included some sample programs that you can use for testing.

## References

-   <https://en.wikipedia.org/wiki/Brainfuck>
-   <https://github.com/tsoding/bfjit>
