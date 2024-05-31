# Build
Use
```shell
make
```
in each directory (named copy, shell and sort),
executables `MyCopy`, `ForkCopy`, `PipeCopy`, `shell`, `MergesortSingle` and `MergesortMulti` will be made out in the corresponding directories. You can `cd Copy` for example and test each program.


# Copy
## Usage
```shell
./Copy <InputFile> <OutputFile> 
```
This command can copy the InputFile to OutputFile. For example,
we can use create a file `test.out` with the same content as `test.in`:
```shell
./MyCopy Tobecopied.txt dest1.txt
./ForkCopy Tobecopied.txt dest2.txt
./PipeCopy Tobecopied.txt dest3.txt
```
The program will print message:
```
Time used: 0.070000 second(s)
File copied successfully.
```
And we can verify the result with:
```shell
diff Tobecopied.txt dest1.txt
diff Tobecopied.txt dest2.txt
diff Tobecopied.txt dest3.txt
```
which will print nothing if the two files are the same.


# shell
## Usage
Firstly start the shell server:
```shell
./shell <Port>
```

Then use telnet as a client:
```shell
telnet localhost <Port>
```
Notice that it's allowed to use more than one client at the same time.
After connecting, telnet will receive:
```
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
qzdlogic@chuhengdu-G11CD-K:/home/qzdlogic/qzdlogic/OSD/Project1/shell>
```

You can type command here, for example:
```shell
ls | wc | wc
```
The output is
```
      1       3      24
```

Use command
```
exit
```
to close the connection. 
# mergesort
## Usage
```shell
./MergesortSingle
./MergesortMulti
```
Prepare `data.in` file, and use `./MergesortSingle` or `./MergesortMulti`
to sort the random numbers. 

The output of the program would be as follows
```
n = 2000000
Time taken by single pthread: 0.367258 second(s)
```
for ./MergesortSingle and
```
n = 2000000
Time taken by single pthread: 0.488551 second(s)
```
for ./MergesortMulti