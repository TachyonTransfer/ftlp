# Faster-Than-Light Protocol: _Tachyon's Fast, BSD-Style Socket API_

UDT-Tachyon is an application level data transfer protocol designed to move large datasets at high speed across a network. UDT-Tachyon continues the work of UDT, and uses UDP to transfer data, but uses its own  congestion control algorithms.

It is best suited for Long Fat Networks (LFNs) as well as use cases for moving large files cross-country or cross-continent. For a technical deep dive check out our blog posts:

## Quickstart:

You'll need two machines, a sender (client) and receiver (server)

After installing the repo on both machines

make

#### on sender:

./sendfile [port number]

#### on receiver:

./recvfile [IP of sender] [port number of sender] [file name on sender] [new name of file on receiver]

### An example to clarify:

#### on sender:

./sendfile 9005

#### on receiver:

./recvfile 3.1.23.140 9005 temp_1GB_file temp_1GB_file

After a file transfer is complete you will see stats on the file speed and time.
