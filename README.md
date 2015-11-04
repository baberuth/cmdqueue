High performance command queue handling in C


TODO:
the pthread creation in the cmdqueue doesn't follow the correct rules for UNIX signal handling. Please
use pthread masking to block all signals and keep 1 thread in place to handle all signals.
