#include "loader.h"

/*
Team Member Details:

Swaraaj Krishna 2024574
Prerak Tanwar   2024436

This is a C program written from scratch for Simple loader for 32 bit.

Contributions:

Prerak Tanwar: 1) Loading entire binary content into the mem from elf file by using lseek,read,open,malloc.
               2) Iterating through the phdr tbale and find section of PT_LOAD tyoe that contains the address of the entrypoint method in fib.c using lseek and read.

Swaraaj Krishna: 1)Allocate memory of the size "p_memsz" using mmap function and copy the segment content using lseek,read,mmap
                 2)Navigate to the entrypoint address into the segment load into the memory.
                 3)Typecast the address to that func ptr mathcing "_start" method in fib.c.
                 4)Call the "_start" method and print the value returned from the "_start".










*/

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;


void loader_cleanup(){
    free(ehdr);
    free(phdr);
    close(fd);
}

void load_and_run_elf(char **argv){
    fd = open(argv[1],O_RDONLY);
    // checking if file can open
    if(fd<0){
        printf("Unable to Opne file");
    }

    // 1. Loading entire binary content into the mem from elf file
    ehdr = malloc(sizeof(Elf32_Ehdr));
    //checking if mem alloc for elf header succesful
    if(ehdr==NULL){
        printf("mem alooc failed for elf header");
    }

    size_t a = read(fd,ehdr,sizeof(Elf32_Ehdr));
    //checking if the elf header was read completely
    if(a != sizeof(Elf32_Ehdr)){
        printf("Unable to read elf header");
    }

    phdr = malloc(ehdr->e_phentsize*ehdr->e_phnum);
    //checking if the mem alloc for pht was successfull
    if(phdr==NULL){
        printf("Mem alloc error");
    }

    lseek(fd,ehdr->e_phoff,SEEK_SET);
    size_t b = read(fd,phdr,ehdr->e_phentsize*ehdr->e_phnum);
    //checking if prog header table was read completely
    if(b!= ehdr->e_phentsize*ehdr->e_phnum){
        printf("Unable to read pht table");
    }
    //2.Iterating through the phdr tbale and fin section of PT_LOAD tyoe that contains the address of the entrypoint method in fib.c
    void *address = NULL;

    for(int i =0;i<ehdr->e_phnum;i++){
        if(phdr[i].p_type == PT_LOAD){
    
    //3.Allocate memory of the size "p_memsz" using mmap function and copy the segment content.
            void *seg = mmap(NULL,phdr[i].p_memsz,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,0,0);
            //Checking is mem mapping was success
            if(seg == MAP_FAILED){
                perror("mmap not working");
            }
            lseek(fd, phdr[i].p_offset, SEEK_SET);
            read(fd, seg, phdr[i].p_memsz);


            unsigned int segment_i = phdr[i].p_vaddr;
            unsigned int segment_f = phdr[i].p_vaddr+phdr[i].p_memsz;
            unsigned int entry = ehdr->e_entry;

    //4. Navigate to the entrypoint address into the segment load into the memory.
            if(entry >= segment_i && entry <segment_f){
                size_t offset = ehdr->e_entry - segment_i;
                address = (char*)seg +offset;

            }
        }
        
    }
    //Checking if entry pt address was found correctly
    //5.Typecast the address to that func ptr mathcing "_start" method in fib.c.
    //6. Call the "_start" method and print the value returned from the "_start".
    int (*_start)()= (int(*)())address;
    int result = _start();
    printf("User _start return value = %d\n",result);
   
}


int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        return 1;
    }
    //1.check for input ELF file -> i did this in the load_and_run()
    //2.pass to loader for carrying out the exec/loading.

    load_and_run_elf(argv);
    //3.cleanup routine
    loader_cleanup();
    return 0;
}