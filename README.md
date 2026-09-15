# Mini Database Engine

A lightweight database engine written in C++.

This project implements a small command-line database with:

-   A SQL-like command interface
-   Statement parsing and execution
-   Persistent storage in a binary database file
-   A Pager for page-based file storage
-   A B+ Tree for indexing and record lookup
-   Insert, select, and delete operations

## Architecture

The project is organized into a few main components:

``` text
User
  ↓
main.cpp
  ↓
Parser
  ↓
Executor
  ↓
Table
  ↓
B+ Tree
  ↓
Pager
  ↓
Database file
```

### Components

-   **Parser** --- reads commands entered by the user and converts them
    into statements.
-   **Executor** --- performs the requested operation.
-   **Table** --- connects the database components and handles row
    serialization/deserialization.
-   **B+ Tree** --- stores records in sorted order and provides
    efficient lookup by ID.
-   **Pager** --- manages fixed-size pages between memory and the
    database file.
-   **Database file** --- stores the database persistently on disk.

## Requirements

You need a C++ compiler with C++17 support.

For example:

-   GCC / g++
-   MinGW on Windows
-   Clang

## How to Run

### 1. Clone the repository

``` bash
git clone <YOUR_GITHUB_REPOSITORY_URL>
cd <YOUR_REPOSITORY_FOLDER>
```

Replace the placeholders with your actual GitHub repository URL and
folder name.

### 2. Compile the project

From the project directory, run:

``` bash
g++ -std=c++17 main.cpp Parser.cpp Executor.cpp Table.cpp Pager.cpp BPlusTree.cpp -o database
```

On Windows, this creates:

``` text
database.exe
```

### 3. Start the database

The program requires a database filename as a command-line argument.

``` bash
./database my_database.db
```

On Windows PowerShell:

``` powershell
.\database.exe my_database.db
```

If the database file does not already exist, the program creates it
automatically.

## Available Commands

### Insert

Insert a row using:

``` text
insert <id> <username> <email>
```

Example:

``` text
insert 1 ayaan ayaan@example.com
insert 2 alex alex@example.com
insert 3 john john@example.com
```

Each ID must be unique.

### Select all records

``` text
select
```

Example output:

``` text
(1, ayaan, ayaan@example.com)
(2, alex, alex@example.com)
(3, john, john@example.com)
```

### Select by ID

``` text
select <id>
```

Example:

``` text
select 2
```

Output:

``` text
(2, alex, alex@example.com)
```

If the record does not exist:

``` text
Record not found.
```

### Delete by ID

``` text
delete <id>
```

Example:

``` text
delete 2
```

Output:

``` text
Record deleted.
```

### Exit

``` text
.exit
```

This exits the database program and flushes loaded pages to the database
file.

## Example Session

``` text
db > insert 1 ayaan ayaan@example.com
Executed.

db > insert 2 alex alex@example.com
Executed.

db > insert 3 john john@example.com
Executed.

db > select
(1, ayaan, ayaan@example.com)
(2, alex, alex@example.com)
(3, john, john@example.com)

db > select 2
(2, alex, alex@example.com)

db > delete 2
Record deleted.

db > select
(1, ayaan, ayaan@example.com)
(3, john, john@example.com)

db > .exit
```

## How Storage Works

The database uses fixed-size pages.

The **Pager** loads pages from the database file into memory when they
are needed and writes loaded pages back to disk.

The **B+ Tree** organizes records by their ID:

``` text
             Internal Node
             /     |      \
            /      |       \
        Leaf     Leaf      Leaf
        ↓         ↓         ↓
      records   records   records
```

Leaf nodes contain the actual keys and rows. The leaf nodes are linked
together, allowing the database to scan all records sequentially.

When a leaf becomes full during insertion, the B+ Tree can split the
leaf and update the parent structure.

## Current Limitations

This is a lightweight educational database engine rather than a full SQL
database.

Current limitations include:

-   Only `insert`, `select`, and `delete` operations are supported.
-   Records are identified by an integer ID.
-   Delete does not perform B+ Tree rebalancing.
-   The database has a fixed maximum number of pages.
-   The parser supports a small SQL-like command syntax rather than full
    SQL.

## Project Structure

``` text
.
├── main.cpp
├── Parser.cpp
├── Executor.cpp
├── Table.cpp
├── BPlusTree.cpp
├── Pager.cpp
└── README.md
```

The corresponding header files should also be present in the project
directory.

## Persistence

The database is stored in the file supplied when starting the program:

``` bash
./database my_database.db
```

Running the program again with the same database filename loads the
existing database contents.

For example:

``` bash
./database my_database.db
```

Insert some records, exit with:

``` text
.exit
```

Then run:

``` bash
./database my_database.db
```

The previously stored records can be queried again.

## License

This project is intended for learning and experimentation.
