# Mini Database Engine

A lightweight educational database engine written in C++.

This project implements a small command-line database with:

-   SQL-like commands
-   Statement parsing and execution
-   Persistent storage in a binary database file
-   A Pager for page-based storage
-   A B+ Tree for indexing and record lookup
-   Insert, Select, Update, and Delete operations

------------------------------------------------------------------------

## Architecture

The project follows this basic flow:

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
-   **Executor** --- performs the requested database operation.
-   **Table** --- connects the database components and handles row
    serialization/deserialization.
-   **B+ Tree** --- organizes records by ID and efficiently finds the
    leaf/page containing a record.
-   **Pager** --- loads fixed-size pages from the database file into
    memory and writes modified pages back to disk.
-   **Database file** --- provides persistent storage.

### B+ Tree and Pager

A useful way to think about the two is:

``` text
B+ Tree → "Where is the record?"
Pager   → "Give me that page."
```

The B+ Tree searches through its nodes and returns a `Cursor` containing
the page number and cell number. The Pager then provides the
corresponding page in memory.

Leaf nodes contain the actual keys and rows. Leaf nodes are linked
together so that the database can scan records sequentially.

When a leaf becomes full during insertion, the B+ Tree can split the
leaf and update the parent structure.

------------------------------------------------------------------------

## Project Structure

``` text
.
├── main.cpp
├── Parser.cpp
├── Parser.h
├── Executor.cpp
├── Executor.h
├── Table.cpp
├── Table.h
├── BPlusTree.cpp
├── BPlusTree.h
├── Pager.cpp
├── Pager.h
└── README.md
```

------------------------------------------------------------------------

## Requirements

You need a C++ compiler with C++17 support.

Examples:

-   GCC / g++
-   MinGW on Windows
-   Clang

------------------------------------------------------------------------

## How to Run

### 1. Compile

From the `src` directory:

``` powershell
g++ *.cpp -o dbms
```

This creates:

``` text
dbms.exe
```

### 2. Start the database

The program requires a database filename:

``` powershell
.\dbms.exe test.db
```

If `test.db` does not exist, the database creates it.

Running the program again with the same filename loads the existing
database contents.

------------------------------------------------------------------------

# Available Commands

## Insert

Syntax:

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

------------------------------------------------------------------------

## Select All

Syntax:

``` text
select
```

Example output:

``` text
(1, ayaan, ayaan@example.com)
(2, alex, alex@example.com)
(3, john, john@example.com)
```

------------------------------------------------------------------------

## Select by ID

Syntax:

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

------------------------------------------------------------------------

## Update

The UPDATE feature changes the username and email of an existing record
while keeping its ID unchanged.

Syntax:

``` text
update <id> <new_username> <new_email>
```

Example:

``` text
insert 1 a am
update 1 b bm
select 1
```

Output:

``` text
(1, b, bm)
```

### How UPDATE works

The ID is the B+ Tree key.

For:

``` text
update 1 b bm
```

the database:

``` text
1. Finds key 1 using the B+ Tree
2. Gets the leaf page containing key 1
3. Finds the cell containing key 1
4. Replaces the stored Row data
5. Keeps the B+ Tree key and location unchanged
```

Conceptually:

``` text
Before:

B+ Tree
   ↓
key = 1
   ↓
(1, a, am)


After:

B+ Tree
   ↓
key = 1
   ↓
(1, b, bm)
```

The B+ Tree does not need restructuring because the key (`1`) has not
changed.

------------------------------------------------------------------------

## Delete

Syntax:

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

The current implementation removes the row from the leaf by shifting
later cells left.

It does **not** perform B+ Tree rebalancing after deletion.

------------------------------------------------------------------------

## Exit

Syntax:

``` text
.exit
```

Example:

``` text
db > .exit
```

This exits the database program and flushes loaded pages to the database
file.

------------------------------------------------------------------------

# Example Session

``` text
db > insert 1 ayaan ayaan@example.com
Executed.

db > insert 2 alex alex@example.com
Executed.

db > select
(1, ayaan, ayaan@example.com)
(2, alex, alex@example.com)

db > update 1 ayaan_new ayaan_new@example.com
Executed.

db > select 1
(1, ayaan_new, ayaan_new@example.com)

db > delete 2
Record deleted.

db > select
(1, ayaan_new, ayaan_new@example.com)

db > .exit
```

------------------------------------------------------------------------

# Persistence

The database is stored in the `.db` file supplied when starting the
program.

For example:

``` powershell
.\dbms.exe test.db
```

After inserting records, exit with:

``` text
.exit
```

Then start the program again:

``` powershell
.\dbms.exe test.db
```

The previously stored records can be queried again.

------------------------------------------------------------------------

# Windows Development Issue: Application Control / Smart App Control

While developing this project on Windows, a locally compiled executable
may sometimes be blocked before the program starts.

For example:

``` text
Program 'dbms.exe' failed to run:
An Application Control policy has blocked this file
```

This is **not a C++ compilation error**.

If compilation succeeds:

``` powershell
g++ *.cpp -o dbms
```

but running the executable fails with the Application Control message,
Windows is blocking the executable before `main()` executes.

### How this was diagnosed

The following command can be used to inspect the executable's signature:

``` powershell
Get-AuthenticodeSignature .\dbms.exe
```

A locally compiled GCC executable may show:

``` text
Status
------
NotSigned
```

That alone is not an error; it simply means the executable does not have
a digital signature.

The Windows Code Integrity event log can provide the actual reason for
an execution block:

``` powershell
Get-WinEvent -LogName "Microsoft-Windows-CodeIntegrity/Operational" -MaxEvents 20 |
    Format-List TimeCreated,Id,Message
```

In this development environment, the log reported events such as:

``` text
Id: 3118
Smart App Control Block Details
```

and:

``` text
Id: 3033
Code Integrity determined that PowerShell attempted
to load the executable that did not meet the
Enterprise signing level requirements.
```

It also reported event `3077`, indicating that the executable did not
meet the required signing level or violated a code integrity policy.

### Important conclusion

If both an old executable and a newly compiled executable are blocked,
while `g++` itself successfully creates the `.exe`, the issue is with
the Windows execution/security policy rather than the database code.

Do **not** immediately change or remove C++ code to fix this error.

First check:

``` text
Windows Security
    ↓
App & browser control
    ↓
Smart App Control
```

If Smart App Control is enforcing a policy, it may block locally
compiled unsigned executables.

------------------------------------------------------------------------

# Current Limitations

This is a lightweight educational database engine rather than a full SQL
database.

Current limitations include:

-   Small SQL-like command syntax rather than full SQL
-   Records are identified by an integer ID
-   IDs must be unique
-   Delete does not perform B+ Tree rebalancing
-   Fixed-size pages
-   Fixed maximum page capacity
-   UPDATE changes the row associated with an existing ID; it does not
    change the ID itself
-   No transactions or rollback system
-   No concurrent multi-user access

------------------------------------------------------------------------

# Learning Goals

This project demonstrates several important database and systems
concepts:

-   Command parsing
-   Query execution
-   Data structures
-   B+ Trees
-   Binary search
-   Cursors
-   Page-based storage
-   Memory layout
-   Serialization and deserialization
-   File I/O
-   Persistent storage
-   Record insertion
-   Record lookup
-   Record deletion
-   Record updating
-   Basic database architecture

------------------------------------------------------------------------

# License

This project is intended for learning and experimentation.
