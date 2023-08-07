How to build:

    - the makefile in the outermost directory can run all of q2, q4, and q5
    - 'make q2' will run q2
    - 'make q4' will run q4
    - 'make q5' will run q5
    - in order to customize the input arguments, you will need to go into each question's makefile and change 
      the command line input manually (more about this in the unusual things section below)
    - make clean will clean q2, q4, and q5 at the same time. no need to clean each individually
    - there is NOT a functionality to run all questions at once. they have to be run individually
    

Unusual things that the TA should know:

    - the outer makefile simply calls the makefile in the respective 'apps/q?' directory, so those makefiles
      must be changed in order to change the input args
    - for example, if you would like to change the number of producer/consumer for q2, you would need to 
      go into "apps/q2/Makefile" and change the command manually on the 'run:' line at the bottom (the same
      would apply to q4 or q5 in their respective directories)
    - for q5, the first argument is the number of NO and the second is the number of H2O
    - for q2 and q4, the only argument is the number of producers/consumers
    - the arguments that are in our submission for each question are:
        * q2 = 3 producers/consumers
        * q4 = 3 producers/consumers
        * q5 = 3 NO and 4 H2O

External sources: 

    -Lab hours and TA help