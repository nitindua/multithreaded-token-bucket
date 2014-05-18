Documentation for Warmup Assignment 2
=====================================

+-------+
| BUILD |
+-------+

Comments: To build the program, please type make.

+---------+
| GRADING |
+---------+

Basic running of the code : 100 out of 100 pts

Missing required section(s) in README file : No
Cannot compile : Can compile
Compiler warnings : None
"make clean" : Yes, Removes all object and executable files (*.o and warmup1)
Segmentation faults : None
Separate compilation : Yes
Using busy-wait : No
Handling of commandline arguments:
    1) -n : Yes, B must be a positive integer and smaller than 2147483647
    2) -lambda : Yes, lambda must be a positive real number with minimum value = 0.1
    3) -mu : Yes, mu must be a positive real number with minimum value = 0.1
    4) -r : Yes, r must be a positive real number with minimum value = 0.1
    5) -B : Yes, B must be a positive integer and smaller than 2147483647
    6) -P : Yes, P must be a positive integer and smaller than 2147483647
Trace output :
    1) regular packets: Yes, 6 lines for each with correct information
    2) dropped packets: Yes, 1 line for each with correct information
    3) removed packets: Yes, 1 line for each with correct information when Ctrl+C interrupt occurs
    4) token arrival (dropped or not dropped): Yes, 1 line for each with correct information for both cases of dropped and not dropped
Statistics output :
    1) inter-arrival time : Yes, calculated using measured inter-arrival time for packets
    2) service time : Yes, calculated using measured service time
    3) number of customers in Q1 : Yes, calculated using measured time for packets in Q1
    4) number of customers in Q2 : Yes, calculated using measured time for packets in Q2
    5) number of customers at a server : Yes, calculated using measured time for packets in Server
    6) time in system : Yes, calculated using measured time in system 
    7) standard deviation for time in system : Yes, calculated as per formula
    8) drop probability : Yes, calculated for both token drop and packet drop
Output bad format : As per format mentioned in the specification. All timestamps on the left line up
Output wrong precision for statistics (should be 6-8 significant digits) : As per requirement in specification. Using %.6g to get correct precision
Large service time test : Pass
Large inter-arrival time test : Pass
Tiny inter-arrival time test : Pass
Tiny service time test : Pass
Large total number of customers test : Pass
Large total number of customers with high arrival rate test : Pass
Dropped tokens test : Pass
Cannot handle <Cntrl+C> at all (ignored or no statistics) : Pass, have handled
Can handle <Cntrl+C> but statistics way off : Pass, statistics are accurate
Not using condition variables and do some kind of busy-wait : Not using busy-wait
Synchronization check : Using a single mutex for both queues and token bucket, as per specification
Deadlocks : None

+------+
| BUGS |
+------+

Comments: None were encountered before submitting

+-------+
| OTHER |
+-------+

Comments on design decisions: As per specification
Comments on deviation from spec: None

