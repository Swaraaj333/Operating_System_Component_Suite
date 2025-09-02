# group-48
OS ASSIGNMENT 1 Section A Simple loader
Swaraaj Krishna 2024574 Prerak Tanwar 2024436

loader_cleanup
cleans the memory used by ehdr and phdr and closes the fd file which we opened

load_and_run_elf

file handling
a) opens the elf file

ELF header setup
b) makes memory for ehdr and then reads it
makes memory for phdr and then reads it
then checks that these memory allocations were successful or not

program header table
c) we use lseek to jump to the start of pht then read copies it to the memory so that we can iterate through it

initialization
d) initialize the address as NULL

segment loading
e) iterate through pht and load the ptype which is mentioned as load type and we then use seg and mmap to copy the segment content
we also check whether the mapping was successful or not

entry point
f) then we check that entry is in the range of the entrypoint address mentioned in the elf header table
after finding it we navigate it into the segment load or memory

execution
g) we then typecast this address to that function pointer matching start and call start and call this function in main
