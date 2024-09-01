# Build
Use
```shell
make
```
in each directory (named step1 and step2(3)) or just in the root directory,
executables `BDS`, `BDC_command`, `BDC_random` in step1 and `FS`, `FC` in step2(3) will be made out in the corresponding directories. You can `cd step1` for example and test each step.


# step1
## Usage
```shell
./BDS <disk-like file name> <num of cylinders> <num of sectors> <track to track delay> <port> 
./BDC_command <port>

./BDS kk.txt 128 128 2 10356
./BDC_command 10356

```
For example, as a command client, we implement the BDC_command, set the port as 10356, disk-like file name as kk.txt, 128 cylinders and 128 sectors per cylinder.
Then we input orders below:
```shell
I
R 0 0
W 0 0  5 ttttt
R 0 0 
```
The following message will be printed on the screen:
```
128 128

Yes
ttttt
```



Then we can also try the random-command client, we can use following order to set 10 random orders, then we will see output as follows:

``` shell
./BDC_random <num of random orders> <port>

./BDC_random 10 10356
```

```
cylinders: 128, sectors_per_cylinder: 128, instructions: 10
W 8 87 256 7a>zJ%aO*QEPoM&DPe3fFu)f<|U0:6TSoG@X8zUL
Yes
W 127 12 256 pC,i!cc~7RgZu<#^<dW7O#<z|
Yes
R 100 22

R 52 54

W 11 39 256 r74=ZMY~)eij$Jy     eANXf)nb3k8 z SRzc-(PcF&h/O_<ua]VN/isj ^^z/2;N1#`Ye/0J@p<lX#GXqJ]m~!*!.g][8J|q9{w
6/J4lH-IRI1N b{YvFP^I asm|n|0sah#rD<A##@4k$F}'ED0r
Yes
@qy9wkxLgaTsyG_qGx>vj!l&kWHAH5.eDsvZ:S]TbJdqDy  zxa`mx:[[datne0/Ll[P-! Vd LW    D U0 S  W:1LQj43nkYg*/W$n
Yes
R 73 24

W 43 62 256 o3JY=8@[)   Ko:     ij<p{C(H63)n)mq}}C%H+Je o gCj#W%J;[rtwY)H<Vm0tXJdy |p%j:~p
Yes
R 119 56

W 26 64 256 ji}$VBj1(D>;@c$ <TCJJ4I(;cx8c.(l47NfRepvI4
Yes
```

In the output message, at first we can see the parameters of the random server(the number of cylinders and sectors are default, the number of orders are setted in the order). Then, we can see both orders and the responses of the server.Because the number of cylinders and sectors are too big that just 10 orders can't read every sector actually.

# step2
## Usage
First,  start the disk server:

```shell
./BDS kk.txt 128 128 2 10356
```

Then use FS as a client:

```shell
./FS 127.0.0.1 10356 12356
```
Here we set 12356 as the new file system port for FC to connect.

```shell
./FC 127.0.0.1 12356
```

After connecting, we can use `format` order to initialize the file system.

We can input orders as follows:
```
f
mkdir dir1
mk file1
```

You can see corresponding reply as well as your inputs as follows:

```shell
f
Format the disk successfully!
>$
mkdir dir1
Create directory successfully! 
>$
mk file1
Create file successfully! 
>$
```

The">$" is an indicator informing you that you can continue your input. 

Then use command

```shell
ls
```
The output would be like:

```shell
#dir1
 file1
>$
 ```

I use '#' to label the directories because I allow a file and a directory having the same name, which can make changing path to another directory easily.

You can use following orders to check if the file or directory is deleted successfully.

```shell
rm file1
ls
rmdir dir1 
ls
mkdir dir
ls
```

You will see that 

```shell
rm file1
Remove file successfully! 
>$
ls
# dir1
>$
rmdir dir1
Remove directory successfully! 
>$
ls
>$
mkdir dir
Create directory successfully! 
>$
ls
# dir
>$
```

And following orders are for checking `cd`, `pwd`, `w`, `i`,`cat`, `r`, `d`, `stat` and `shocc`:

```shell
shocc 0
pwd
cd dir
pwd
mkdir dir1
cd dir1
pwd
mk file
w file 5 fhfhfhf
cat file
r file 2 2
i file 2 5 66666
cat file
d file 0 3
cat file
shocc 0
stat file
```

Then you can see output as follows:
```shell
shocc 0
The disk occupancy of block 0 is as follows: bitmap[0]: 
ffffc000000000000000000000000000800000000000000000000000000
00000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000
00000000000000000000
>$
pwd
/root/
>$
cd dir
Change directory successfully!
>$
pwd
/dir/
>$
mkdir dir1
Create directory successfully! 
>$
cd dir1
Change directory successfully!
>$
pwd
/dir/dir1/
>$
mk file
Create file successfully! 
>$
w file 5 fhfhfhf
Write file successfully! 
>$
cat file
fhfhf
>$
r file 2 2
fh
>$
i file 2 5 66666
Insert data successfully! 
>$
cat file
fh66666fhf
>$
d file 0 3
Delete data successfully! 
>$
cat file
6666fhf
>$
shocc 0
The disk occupancy of block 0 is as follows: bitmap[0]: 
ffffc000000000000000000000000000e00000000000000000000000000
00000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000800000000000
00000000000000000000
>$
stat file
The file name is file
The file size is 7
The file type is file
The file link count is 1
The file last change time is Fri May 31 23:07:47 2024

The file locates at 1 cylinder 2 sector
>$
```
In the `w` order, we set the maximum length of data is 5, however, the data we input is longer than 5, so only 5 letters can be writen in the file. Besides, the `root` will be hided in the son directories. We can find that the output are all legal so the implement of these orders are correct! 

We can see the block occupancy easily by `shocc` order!

# step3
The step3 : Multi-user is implemented by just changing a bit code in step2,  so I have not created any new file at all. So the usage is the same as step2.

We can try following inputs:

```shell
userls
adduser test
f test
pwd
mkdir dir
cd dir
pwd
f qzdlogic
pwd
ls
cd dir
mkdir dir
cd dir
pwd
```

And the output is as follows:
```shell
userls
root
qzdlogic
>$
adduser test
Add user successfully! 
>$
f test
Format the disk successfully in user's directory! 
>$
pwd
/test/
>$
mkdir dir
Create directory successfully! 
>$
cd dir pwd
Change directory successfully!
>$
pwd
/dir/
>$
f qzdlogic
Format the disk successfully in user's directory! 
>$
pwd
/qzdlogic/
>$
ls
>$
cd dir
The directory is not exist in current path!
>$
mkdir dir
Create directory successfully! 
>$
cd dir
Change directory successfully!
>$
pwd
/dir/
>$
```
In this test, firstly we check the user list. Then we add a new user `test`, and try to create a new "root" directory. By checking the path we can find that we succeed.
Then in this new root, we create a new directory `dir`. Then we change current path into it, which indicates that the new root directory "test" is really a new root because it will be hided when we are in its son directory. Then we change into another user's own root in the user list we saw at first `qzdlogic`.
In "qzdlogic", we can't find "dir" any more which indicates that it is a brand new root directory!

To sum up, my toy file system has achieved all the mentioned orders and multi-user task!

Here I want to say thanks to my teacher and all TAs. Without the socket-example provided by them, I can't finish project in such a hurry term!



