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




#Citations:

For the typecasting adress step of the assignment 


For understanding the process of how to navigate the entry point address into the segment load into the memory.
I have referred to this website:   https://stackoverflow.com/questions/55564620/jump-to-entry-point-of-elf-from-loader


I understood from this line of code to do this step 

int ret = ((int (*)(int, char **, char **)) elf64Ehdr.e_entry)(1, argv1, argv1);

The ELF HEADERS'S entry point addr  is a memory adress.It is typecasted into a fuction pointer so that we can call it.
And I also make sure that the address lies within the memory range where the PT_LOAD segements are mapped using mmap.
Hence this points to the executable program.

And have understood and implimented it in a simpler way suitable as per our assignment.

