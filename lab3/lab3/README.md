Q2:
-to compile and run our q2 code: 'make q2'
-to edit the number of molecules, go into apps/q2/Makefile and change the final two numbers on the run 
 command at the bottom. the first number is NO and the second number is H2O

Q3:
-to compile and run lottery_test code: 'make q3'
-we have three runnable options: Dynamic Lottery, Static Lottery, and Round Robin
-the macros are located in os/Makefile in the CFLAGS line (we put comments that explain
 each line and mark it as lab 3)
-the macros are LT_SCHED and RR_SCHED to choose between lottery and round robin, then DYNAMIC and 
 STATIC to choose between dynamic lottery and static lottery
-there are three lines in os/Makefile that define the macros for each type of scheduler, so just 
 uncomment the one you want to use and make sure that the other two are commented
-dynamic lottery is the default in our submitted os/Makefile
