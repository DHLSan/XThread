# XThread
XThread: A Hardware-based Multi-core Parallel Programming Environment Using Pthread-like Programming Model	

Team Number: xohw21-136

This project was carried out within the scope of the EEM464 course at the Department of Electrical and Electronics Engineering at Eskişehir Technical University.

## **_Step-by-step Building and Testing Instructions_** ##

Choose a benchmark and follow the steps below.

Creating core IP with HLS:
  1. Open Vitis HLS 2020.1
  2. Create New Project
  3. Name of the top function should be "core_2". Replace the contents of the core_2.cpp with HLS/core/core2.cpp
  4. Addind files the HLS/core/xthread.cpp and HLS/core/xthread.h to the your project.
  5. Click C synthesis.
  6. When synthesis is finished, click Export RTL.

Creating control_unit IP with HLS:
  1. Open Vitis HLS 2020.1
  2. Create New Project
  3. Name of the top function should be "control_unit_1". Replace the contents of the control_unit_1.c with HLS/control_unit/control_unit_1.c
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
  13. Make neccessary connections as seen in block_design.pdf, make sure the connections are exactly the same with block_design.pdf
  14. Click Generate Bitstream
  15. Right click on design file and click Create HDL Wrapper.
  16. Right click on design and click Generate Output Products.
  17. Click on Generate Bitstream.

When generate bitstream finished, export hardware (choose include bitstream option) and open Vitis 2020.1.
  1. Open Xilinx Vitis 2020.1, choose workspace for new project
  2. Create the platform project, select the .xsa file that are created after the export hardware step.
  3. Create the application project, select created platform project and select a template(Hello World) to create your project.
  4. Replace the contents of the Hello world project with  VITIS/main.cpp 
  5. Addind files the VITIS/xthread.c and VITIS/xthread.h to the your project.
  6. Addind files the VITIS/timerHelper.c and VITIS/timerHelper.h to the your project.
  7. Ready to run.
  8. Change number of thread and record execution time.
