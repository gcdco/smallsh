Name: George Duensing
Course: cs344 Operating Systems
Project 3: smallsh
Date: February 09, 2021 (winter)

To Compile:
run 'make' to produce executable.

(gcc -std=gnu99 -g main.c smallsh.c -lm -o smallsh)

Note: can also execute w/ 'make run'

clean up .o files and executable:
run: make clean

--

To run:
./smallsh

To run with p3testscript:
To run the script, place it in the same directory as your compiled shell, chmod it (chmod +x ./p3testscript) and run this command from a bash prompt:

$ ./p3testscript 2>&1
or

$ ./p3testscript 2>&1 | more
or

$ ./p3testscript > mytestresults 2>&1
