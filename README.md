# Introduction

The project aims to implement a distributed file system from scratch, inspired by the structure of a Network File System (NFS). Comprising clients, a Naming Server (NM), and Storage Servers (SS), the system orchestrates seamless file interactions across a network. The NM serves as the directory service, efficiently directing clients to the specific SS where requested files reside. Storage Servers manage the storage, responding to commands from the NM and clients, supporting file operations such as reading, writing, and deletion. The system is designed to accommodate concurrent client access, efficient search mechanisms, error handling, and features like redundancy and replication for fault tolerance. With a focus on modularity and early planning, the project encourages the team to divide tasks, define calls, and make necessary assumptions to deliver a well-organized and functional distributed file system.

# Contributors

### 1. Mohak Somani
- **Roll No :**  2022101088
- **GitHub UserID :** MohakSomani
- **Email :** mohak.somani@students.iiit.ac.in
- 
### 2. Prakhar Singhal
- **Roll No :** 2022111025
- **GitHub UserID :** legend479
- **Email :** prakhar.singhal@research.iiit.ac.in

### 3. Hemang Jain
- **Roll No :**  2022101086
- **GitHub UserID :** hemang-n00b
- **Email :** hemang.jain@students.iiit.ac.in


# How to Initialize/Run the System

### Naming Server
- Open Terminal in the `naming_server` folder
- Run `make clean` to clear up the un-necessary object files and free the Global Ports
- Run `make` to create and executible for the Naming Server
- Run the executible as `./NS`

### Storage Server
- Open terminal in the main directory , i.e. , `final-project-51` 
- Run `gcc ss.c -o ss` to create the executible for Storage Servers
- Now in all the directories where you want to run a storage server from , copy paste this executible there 
- Finally Run it in each directory with `./ss <Port for Client Connection> <Port for Naming Server connection>`

### Client
- Open terminal in the main directory , i.e. , `final-project-51`
- Run `gcc client.c -o c` to create the executible for the Client
- Now for each client you want , run `./c` 

# API Calls

### 1. READ
Read and display the contents of a file at the specified path.

Input Format : **READ < PATH >**

### 2. WRITE
Append (A) or overwrite (O) data to the file at the given path.

Input Format : **WRITE < A/O > < Path > < Data >**

### 3. INFO
Retrieve the file/directory 's size and permissions at the specified path

Input Format : **INFO < Path >**

### 4. CREATE
Create a new file (F) or directory (D) with the given name at the specified path.

Input Format : **CREATE < F/D > < Name > < Path >**

### 5. DELETE
Delete the file or directory at the specified path.

Input Format : **DELETE < Path >**

### 6. COPY
Copy a file from the source path to the destination path.

Input Format : **COPY < Source Path > < Destination Path >**

### 7. LIST
List the contents of the specified directory. If no path is given, it lists the MOUNT folder.

Input Format : **LIST < Path >**

# Error Codes

0 : Success / OK

## 1. Client Side Errors:

101: Error Connecting to Naming Server

102: Error Connecting to Storage Server

103: Invalid Flags

104: Invalid Command

105: Connection Timed Out

## 2. Naming Server Side Errors:

201: Path Not Found

202: Command Not Found

203: Transfer Error

205: Request Forward Error

205: Mount Del

## 3. Storage Server Side Errors:

301: File does not exist

302: File not accessible

303: File Creation Unsuccessful

304: Directory Creation Unsuccessful

# Global Ports Used


1. 8080: Naming Server (Server side communication)

2. 8081: Naming Server (Client side communication)

  
# POSIX Libraries Used

1. stdio.h
2. stdlib.h
3. string.h
4. unistd.h
5. arpa/inet.h
6. sys/select.h
7. sys/socket.h
8. stdbool.h
9.  pthread.h
10. fcntl.h
11. math.h
12. strings.h
13. sys/stat.h
14. sys/wait.h
15. sys/times.h
16. dirent.h
17. semaphore.h
18. sys/ioctl.h
19. errno.h
20. poll.h
21. sys/types.h
22. stdint.h
23. time.h

# References Used

- [Detect a non-graceful disconnect in C#][1]
- [Prevent scanf to wait forever for an input character][2]
- [Reasons for connection refused errors][3]
- [Significance of 5381 and 33 in the djb2 algorithm][4]
- [Check if a TCP connection was closed by peer gracefully or not][5]
- [Gist on handling TCP connection closed by peer][6]
- [Linux `poll` system call][7]
- [Linux `recv` system call][8]
- [How does the `poll` function work in C?][9]
- [Jenkins hash function on Wikipedia][10]

Additional resources:

- [CS3301 OSN Course Policy][11]
- [Doubt Document - HackMD][12]

[1]: https://stackoverflow.com/questions/36622823/detect-a-non-gracefull-disconnect-in-c-sharp
[2]: https://stackoverflow.com/questions/21197977/how-can-i-prevent-scanf-to-wait-forever-for-an-input-character
[3]: https://stackoverflow.com/questions/2333400/what-can-be-the-reasons-of-connection-refused-errors
[4]: https://stackoverflow.com/questions/1579721/why-are-5381-and-33-so-important-in-the-djb2-algorithm
[5]: https://stackoverflow.com/questions/38815832/how-to-check-if-a-tcp-connection-was-closed-by-peer-gracefully-or-not-without-re
[6]: https://gist.github.com/RabaDabaDoba/145049536f815903c79944599c6f952a
[7]: https://linux.die.net/man/2/poll
[8]: https://linux.die.net/man/2/recv
[9]: https://stackoverflow.com/questions/9167752/how-does-the-poll-function-work-in-c
[10]: https://en.wikipedia.org/wiki/Jenkins_hash_function
[11]: https://karthikv1392.github.io/cs3301_osn/course_policy/
[12]: https://hackmd.io/XTGbmvj6SAuxiJdeBmV68A