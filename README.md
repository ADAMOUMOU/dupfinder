
# dupfinder
Linux tool to find duplicated files recursively in a folder or between 2 folders using their hashes.

## How to compile
No external libraries are needed to compile it.
First install the build tools for your distribution:
### Debian/Ubuntu:
```
sudo apt update
sudo apt install build-essential
```
### Fedora:
```sudo dnf install gcc-c++ make```
### Debian/Ubuntu:
```sudo pacman -Syu base-devel```

Then you can compile `main.cpp`:
```g++ -o dupfinder main.cpp -Wall```

## How to use it
Here is the usage:`./dupfinder <folder1> [folder2] [--fast]`

Where:
 - `folder1`: is the folder to check (will check only this one if only that is specified
 - `folder2` (optional): is the folder that will be checked with the first one
 - `--fast` (optional, parameter): will only hash the first 8kb of each files instead of hashing them entirely




