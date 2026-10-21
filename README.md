# Toy DBMS

Toy DBMS is an educational relational database management system written in C. The goal of the project is to explore database internals by implementing core storage and execution components from scratch rather than relying on existing database libraries.

This project is intended as a learning platform for understanding how systems such as PostgreSQL, SQLite, and DuckDB are built internally.

## Features

Current and planned components include:

- Page-based storage engine
- Disk manager
- Asynchronous disk scheduler
- Buffer pool manager (LRU replacement policy)
- Page replacement policies
- Write-ahead logging (WAL)
- Table and tuple management
- Index structures (hash table, B+Tree)
- Transaction support (MVCC)
- Recovery and crash handling
- Query execution engine (volcano)

## Goals

The primary goals of this project are:

- Learn database system architecture
- Implement storage and memory management
- Understand concurrency and synchronization
- Study durability and crash recovery mechanisms

This project prioritizes clarity and educational value over performance.

## Architecture

```
+-------------------+
| SQL Parsing       |
+-------------------+
          |
          v
+-------------------+
| Query Planning    |
+-------------------+
          |
          v
+-------------------+
| Operator Execution|<-----<
+-------------------+      |
          |                |
          v                |
+-------------------+    +-------------------+
| Access Methods    |<---| Concurreny Control|
+-------------------+    +-------------------+
          |                |
          v                |
+-------------------+      |  
| Buffer Pool       |<-----<
+-------------------+      |
          |              +-------------------+
          |              | Recovery          |
          v              +-------------------+                     
+-------------------+      |                   
| Disk Manager      |<-----<
+-------------------+
```

## Building

### Requirements

- GCC
- POSIX-compatible operating system (Linux recommended)
- Make

### Run Tests

```bash
make all
```

Or run individual test suites:

```bash
make list_tests
make table_tests
make replacer_tests
etc
```

## Project Structure

```text
src/
├── buffer/        Buffer pool and replacement policies
├── disk/          Disk manager and scheduler
├── storage/       Pages, tuples, tables
├── recovery/      Logging and recovery
├── common/        Shared utilities
└── include/       Public headers

test/
└── ...            Unit tests
```

## Design Principles

- Prefer simple, understandable implementations.
- Write tests alongside new features.
- Build systems incrementally.
- Separate interfaces from implementations.

## Learning Resources

This project draws inspiration from:

- PostgreSQL
- SQLite
- DuckDB
- BusTub (CMU CS 445 Database Systems)

## Status

This is an educational work in progress. APIs, file formats, and internal designs may change frequently as the project evolves.

## License

MIT License
