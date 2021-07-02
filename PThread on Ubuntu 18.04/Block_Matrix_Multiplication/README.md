# XThread
XThread: A Hardware-based Multi-core Parallel Programming Environment Using Pthread-like Programming Model	

Team Number: xohw21-136

This project was carried out within the scope of the EEM464 course at the Department of Electrical and Electronics Engineering at Eskişehir Technical University.


## **_Step-by-step Building and Testing Instructions_** ##

PThread Paralel Version

1.Create new benchmark_name_pthread.c project in file. 
2.Replace the contents of block_matrix_mul_pthread.c
3.Compile code --> gcc block_matrix_mul_pthread.c -o MyProgram -lpthread
4. Run program-->  ./MyProgram
5. Record execution time
6. Change number of thread in code
7. Repeat steps 3, 4 and 5

Sequential Version

1.Create new benchmark_name_sequential.c project in file. 
2.Replace the contents of block_matrix_mul_sequential.c
3.Compile code --> block_matrix_mul_sequential.c -o MySeqProgram 
4.Run program-->  ./MySeqProgram
5.Record execution time
