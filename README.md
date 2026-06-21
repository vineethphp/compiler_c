# Compiler C

A C language compiler implementation built from scratch.

## Overview

This project is a complete C compiler written in C, designed to understand and implement the core concepts of compiler design including lexical analysis, syntax parsing, semantic analysis, and code generation.

## Features

- **Lexical Analysis**: Tokenization of C source code
- **Syntax Analysis**: Parsing and Abstract Syntax Tree (AST) generation
- **Semantic Analysis**: Type checking and symbol table management
- **Code Generation**: Compilation to machine code or intermediate representation

## Project Structure

```
compiler_c/
├── src/           # Source code
├── include/       # Header files
├── test/          # Test cases
└── Makefile       # Build configuration
```

## Getting Started

### Prerequisites

- GCC or Clang compiler
- Make
- Linux/Unix environment (or Windows with WSL)

### Building

```bash
make
```

### Running

```bash
./compiler <input_file.c> -o <output_file>
```

### Testing

```bash
make test
```

## Usage

Compile a C program:

```bash
./compiler program.c -o program
```

## Examples

Basic usage example:
```c
// hello.c
#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    return 0;
}
```

Compile and run:
```bash
./compiler hello.c -o hello
./hello
```

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Author

**vineethphp**

## Acknowledgments

- Based on compiler design principles and theory
- Inspired by classic compiler architecture patterns
