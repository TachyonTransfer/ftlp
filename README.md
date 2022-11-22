# UDT-Tachyon Protocol: _Tachyon's Fast, BSD-Style Socket API_

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


## For Testing:

You can find our tachyon testing AMI (
Tachyon FTLP/UDT Performance Testing Image) on AWS (N Virginia region) and launch an ec2 instance with it. Once you do that you can test performance

### Add linker path export to run executables
cd ftlp/src/
export LD_LIBRARY_PATH=$(pwd):$$LD_LIBRARY_PATH

#### Add packet loss percentages using iptables, expressed as a fraction: --every flag is denominator, --packet flag is numerator
sudo iptables -A FORWARD -p udp -m statistic --mode nth --every 1000 --packet 1 -j DROP
#### Remove packet loss rules from iptables
sudo iptables -D FORWARD -p udp -m statistic --mode nth --every 1000 --packet 1 -j DROP
#### Set traffic shaping params in env: Delay(ms), Rate(Mbps)
DELAY_MS=20
RATE_MBIT=500
#### Math to determine buffer size, expressed as number of packets
BUF_PKTS=500
BDP_BYTES=$(echo "($DELAY_MS/1000.0)*($RATE_MBIT*1000000.0/8.0)" | bc -q -l)
BDP_PKTS=$(echo "$BDP_BYTES/1500" | bc -q)
LIMIT_PKTS=$(echo "$BDP_PKTS+$BUF_PKTS" | bc -q)
#### Adding queing discipline to loopback interface (lo) with the given params
sudo tc qdisc replace dev lo root netem delay ${DELAY_MS}ms rate ${RATE_MBIT}Mbit limit ${LIMIT_PKTS}
#### Show queuing disciplein on loopback interface
sudo tc qdisc show dev lo
#### Delete queuing disciplein on loopback interface
sudo tc qdisc del dev lo root
