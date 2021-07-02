# XThread
XThread: A Hardware-based Multi-core Parallel Programming Environment Using Pthread-like Programming Model	

Team Number: xohw21-136

This project was carried out within the scope of the EEM464 course at the Department of Electrical and Electronics Engineering at Eskişehir Technical University.

## **_Project Description_** ##
Multi-core programming models including PThreads, OpenMP and MPI allow one to utilize the power of all the existing available cores inside CPUs and GPUs. However, the number of cores, the architecture of execution units in ALUs and memory architectures are fixed in these computing platforms. Programmer needs to describe the program in a way to distribute execution tasks across available cores, with efficient data synchronization, considering the number of available cores. The achievable parallelism is limited with the fixed hardware, it may not be possible to achieve the maximum parallelism of the application.  In our study, we propose XThread, a reconfigurable parallel programming framework that allows one to design and implement application-specific hardware for the application of interest from a Pthread like C-level programming to overcome the limitation of the fixed hardware architectures. XThread shows a speedup 2.32x on block matrix multiplication. Considering the frequency difference between KCU105 Xilinx Kintex UltraScale FPGA and Intel i7 7th generation processor, the XThread library interface is much more advantageous in all experiments. It provides less power consumption. XThread consists of the same programming structures in Pthread and easy to transfer existing Pthread programs in order to speed-up application in reconfigurable hardware, e.g., FPGA.

![](XThread%20Experiments%20on%20KCU105/Images/1.png)
❮img src="XThread%20Experiments%20on%20KCU105/Images/1.png" width="750"  height="375" ❯

## **_Participants_** ##
- Fatma ÖZÜDOĞRU
  - Contact: ozudogrufatma26@gmail.com
- Ersin ABBASOĞLU
  - Contact: ersinabbasoglu@anadolu.edu.tr
## **_Supervisor_** ##
 - Assist. Prof. Dr. İsmail San
    - Contact: isan@eskisehir.edu.tr
## **_Platform and Tools_** ##
- KCU105 Xilinx Kintex XCKU040-2FFVA1156E UltraScale FPGA
- Xilinx Vitis High-Level Synthesis 2020.1
- Xilinx Vivado Design Suit 2020.1
- Xilinx Vitis 2020.1

## **_Report_** ##
Project phases and more detail about project is shared in this repository as XThread_report_xohw21-136.pdf
## **_Video Link_** ##

## **_SoC Architecture_** ##

![](XThread%20Experiments%20on%20KCU105/Images/5.png)

## **_Experimental Results_** ##
![](XThread%20Experiments%20on%20KCU105/Images/2.png)
![](XThread%20Experiments%20on%20KCU105/Images/3.PNG)
![](XThread%20Experiments%20on%20KCU105/Images/4.PNG)
