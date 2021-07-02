# XThread
XThread: A Hardware-based Multi-core Parallel Programming Environment Using Pthread-like Programming Model	

Team Number: xohw21-136

Eskisehir Technical University

## **_Project Description_** ##
Multi-core programming models including PThreads, OpenMP and MPI allow one to utilize the power of all the existing available cores inside CPUs and GPUs. However, the number of cores, the architecture of execution units in ALUs and memory architectures are fixed in these computing platforms. Programmer needs to describe the program in a way to distribute execution tasks across available cores, with efficient data synchronization, considering the number of available cores. The achievable parallelism is limited with the fixed hardware, it may not be possible to achieve the maximum parallelism of the application.  In our study, we propose XThread, a reconfigurable parallel programming framework that allows one to design and implement application-specific hardware for the application of interest from a Pthread like C-level programming to overcome the limitation of the fixed hardware architectures. XThread shows a speedup 2.32x on block matrix multiplication. Considering the frequency difference between KCU105 Xilinx Kintex UltraScale FPGA and Intel i7 7th generation processor, the XThread library interface is much more advantageous in all experiments. It provides less power consumption. XThread consists of the same programming structures in Pthread and easy to transfer existing Pthread programs in order to speed-up application in reconfigurable hardware, e.g., FPGA.

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


## **_Step-by-step Building and Testing Instructions_** ##

--Running the one of the experiments
Creating core IP with HLS:
  1. Open Vitis HLS 2020.1
  2. Create New Project
  3. Name of the top function should be "core_2". Replace the contents of the core_2.cpp with Relevant_Experiment/HLS/core/core2.cpp
  4. Addind files the Relevant_Experiment/HLS/core/xthread.cpp and Relevant_Experiment/HLS/core/xthread.h to the your project.
  5. Click C synthesis.
  6. When synthesis is finished, click Export RTL.

Creating control_unit IP with HLS:
  1. Open Vitis HLS 2020.1
  2. Create New Project
  3. Name of the top function should be "control_unit_1". Replace the contents of the control_unit_1.c with Relevant_Experiment/HLS/control_unit/control_unit_1.c
  4. Click C synthesis.
  5. When synthesis is finished, click Export RTL.
  
Creating SoC Design with All Components:
  1. Open Vivado 2020.1
  2. Create project
  3. Choose KCU105 as destination board
  4. Create block design
  5. Add ip --> Microblaze
  6. Add ip --> Axi_timer
  7. Add ip --> Axi_uart
  8. Add ip --> DDR4
  9. Add source --> 5 Axi_interconnect
  10. Click Window --> Add ip catalog --> click right click on list --> select your the path of the core IP you exported --> click OK
  11. Click Window --> Add ip catalog --> click right click on list --> select your the path of the control_unit IP you exported --> click OK
  12. click right click on block design -->  ip catalog --> select ip files which you created with HLS(12 core and 1 control_unit)
  13. Make neccessary connections as seen in KCU105_Experiments/block_design.pdf, make sure the connections are exactly the same with block_design.pdf
  14. Click Generate Bitstream
  15. Right click on design file and click Create HDL Wrapper.
  16. Right click on design and click Generate Output Products.
  17. Click on Generate Bitstream.

When generate bitstream finished, export hardware (choose include bitstream option) and open Vitis 2020.1.
