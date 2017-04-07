# VirtualFS

Allows reading and writing files to a single level directory virtual filesystem

```
usage: vfs [diskname] [command] [args]...
available commands:
create [size] - create virtual disk with size in kbytes
remove - remove virtual disk
cpto [src] [dest] - copy src file into virtual disk with optional dest name
cpfrom [src] [dest] - copy src file from virtual disk with optional dest name
rm [filename] - remove file with given name from virtual disk
ls - list files on virtual disk
dump - show current usage of data blocks
```
