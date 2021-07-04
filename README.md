# XThread
XThread: A Hardware-based Multi-core Parallel Programming Environment Using Pthread-like Programming Model	

Team Number: xohw21-136

This project was carried out within the scope of the EEM464 course at the Department of Electrical and Electronics Engineering at Eskişehir Technical University.

## **_Project Description_** ##
Multi-core programming models including PThreads, OpenMP and MPI allow one to utilize the power of all the existing available cores inside CPUs and GPUs. However, the number of cores, the architecture of execution units in ALUs and memory architectures are fixed in these computing platforms. Programmer needs to describe the program in a way to distribute execution tasks across available cores, with efficient data synchronization, considering the number of available cores. The achievable parallelism is limited with the fixed hardware, it may not be possible to achieve the maximum parallelism of the application.  In our study, we propose XThread, a reconfigurable parallel programming framework that allows one to design and implement application-specific hardware for the application of interest from a Pthread like C-level programming to overcome the limitation of the fixed hardware architectures. XThread shows a speedup 2.32x on block matrix multiplication. Considering the frequency difference between KCU105 Xilinx Kintex UltraScale FPGA and Intel i7 7th generation processor, the XThread library interface is much more advantageous in all experiments. It provides less power consumption. XThread consists of the same programming structures in Pthread and easy to transfer existing Pthread programs in order to speed-up application in reconfigurable hardware, e.g., FPGA.

![](XThread%20Experiments%20on%20KCU105/Images/1.png)

## **_Participants_** ##
- Fatma ÖZÜDOĞRU
  - Contact: ozudogrufatma26@gmail.com
- Ersin ABBASOĞLU
  - Contact: ersinabbasoglu@anadolu.edu.tr
## **_Supervisor_** ##
 - Assist. Prof. Dr. İsmail San
    - Contact: isan@eskisehir.edu.tr
## **_Platform and Tools_** ##
XThread Experiments:
- KCU105 Xilinx Kintex XCKU040-2FFVA1156E UltraScale FPGA
- Xilinx Vitis High-Level Synthesis 2020.1
- Xilinx Vivado Design Suit 2020.1
- Xilinx Vitis 2020.1
- Clock Frequency: 100 MHz

PThread Experiments:
- Processor	Intel Core i7 7th Gen
- Architecture	x86_64
- Operating System:Linux 
- Ubuntu Version	18.04
- Gcc Version:	6.5.0 
- Clock Frequency:3800MHz


## **_Report_** ##
Project phases and more detail about project is shared in this repository as XThread_report_xohw21-136.pdf
## **_Video Link_** ##

https://youtu.be/lrPrZxK1VS4

## **_Experimental Results_** ##
In the finding minimum experiment, the Pthread library is approximately 1.64 times faster than the XThread library interface when the number of cores is 12. Considering the frequency difference between them, the XThread library interface is much more advantageous. It provides less power consumption.

![](XThread%20Experiments%20on%20KCU105/Images/2.png)

In the gaussian elimination benchmark,the Pthread library is approximately 1.29 times faster than the XThread library interface when the number of cores is 12. In the same way, considering the frequency difference between them, the XThread library interface  is much more advantageous. It provides less power consumption.

![](XThread%20Experiments%20on%20KCU105/Images/3.PNG)

In the Block Matrix Multiplication experiment, while the number of cores is 12, the Xthread library is about 2.32 times faster than the Pthread library. When calculated with the frequency difference between them, Xthread hardware is very advantageous in terms of both speed and low power consumption.

![](XThread%20Experiments%20on%20KCU105/Images/4.PNG)



