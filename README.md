# Multi-Threaded File System With Cache Management

A high-performance file system implementation with multi-threading support, advanced caching, compression, and backup management.

## Quick Start

**Prerequisites:** CMake 3.15+, C++17 compiler

**Build:**
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

**Run:**
```bash
# Main CLI Application
.\build\cli\Debug\mtfs_cli.exe

# Benchmarks
.\build\benchmark\Debug\benchmark_main.exe      # Complete comparison
.\build\benchmark\Debug\fs_benchmark.exe        # File system performance
.\build\benchmark\Debug\real_comparison_benchmark.exe  # Real-world tests

# Tests
.\build\test\Debug\integration_test.exe
```

## Core Features

- **Multi-Threading**: Thread pool, async operations, parallel backup
- **Advanced Caching**: LRU/LFU/FIFO/LIFO policies with prefetching and pinning
- **File Operations**: Create, read, write, copy, move, rename, delete
- **Compression**: Built-in compression with statistics tracking
- **Backup System**: Full/incremental backups with versioning
- **Authentication**: User management with permission controls
- **Performance**: Real-time monitoring and analytics

## CLI Commands

```bash
# User Management
register admin password123 admin
login admin password123

# File Operations  
create-file demo.txt
write-file demo.txt "Hello World!"
read-file demo.txt
copy-file demo.txt backup.txt

# Compression & Backup
compress-file demo.txt
create-backup my_backup
list-backups

# Cache Management
set-cache-policy LRU
pin-file important.txt
cache-analytics

# Statistics
show-stats
exit
```
## Project Architecture

```
├── cli/           # Command-line interface
├── fs/            # Core file system & backup
├── cache/         # Multi-policy caching (LRU/LFU/FIFO/LIFO)
├── storage/       # Low-level storage operations
├── threading/     # Thread pool & async operations
├── common/        # Shared utilities
├── test/          # Test suite
└── benchmark/     # Performance benchmarks
```

## Benchmarks

Performance comparison against standard library:
- **Custom LRU Cache** vs std::unordered_map: 36% faster
- **File Operations** with caching: 21% faster  
- **Compression**: 78% size reduction
- **Real-time metrics**: Hit rates, throughput, timing

## License

MIT License
