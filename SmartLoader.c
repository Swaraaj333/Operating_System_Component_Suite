#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
//extra includes in readme
#include <signal.h>
#include <stdint.h>
#include <errno.h>

// 1 page size = 4 KB
#define PG_SIZE 4096   

//elf header,program header table ptr and file descriptor for elf 
Elf32_Ehdr *ehdr = NULL;
Elf32_Phdr *phdr = NULL;
int fd = -1;

//calculation of pages,page faults, internal fragmnetation,mem size PT_LOAD
int total_pages = 0;
int page_faults = 0;
size_t internal_frag = 0;
size_t tot_pt_loadmem_size = 0;
// for internal fragmentation

//pointer for entrypoint function
int (*entry_func)() = NULL;
size_t entry_point = 0;


//all the function used
void ELF_loader_runner(char **argv);
void run_program(int (*_start)());
void seg_v_handle(int signal, siginfo_t *info, void *context);
void handle_page_fault(uintptr_t addr);
void Cleaner_loader();
int get_prot(uint32_t flags);


int main(int argc, char **argv){
    
    
    if(argc!= 2) {
        printf("incorrect command\n");
        printf("used as: %s elf file\n", argv[0]);
        return 1;
    }


    //segmentation fault signal handling
    struct sigaction sig_a;  
    memset(&sig_a, 0, sizeof(sig_a));
    sig_a.sa_flags = SA_SIGINFO;
    sig_a.sa_sigaction = seg_v_handle; 
    sigemptyset(&sig_a.sa_mask);

    if(sigaction(SIGSEGV, &sig_a, NULL) == -1){
        printf("unabel to sigaction\n");
        return 1;
    }

    //elf file gets loaded then execute
    ELF_loader_runner(argv); 

    //clean up
    Cleaner_loader(); 

    //calculating the internal fragmentation 
    internal_frag = total_pages*PG_SIZE - tot_pt_loadmem_size;
    if ((ssize_t)internal_frag < 0)
        internal_frag = 0;

    //calculations
    printf("total page faults : %d\n", page_faults);
    printf("total page allocations : %d\n", total_pages);
    printf("internal Fragmentation : %zu bytes\n", internal_frag);
    printf("internal Fragmentation : %.3f KB\n", internal_frag / 1024.0);
    printf("calculation is done\n");

    return 0;
}


// protection flags read,write,executable
int get_prot(uint32_t flags){

    int prot = 0;

    if(flags & PF_R){
        prot |= PROT_READ;
    }

    if(flags & PF_W){
        prot |= PROT_WRITE;
    }

    if(flags & PF_X){
        prot |= PROT_EXEC;
    }

    return prot;
}

//reading the elf header and then call entry point, this then runs prog
void ELF_loader_runner(char **argv){ 
      
    fd = open(argv[1], O_RDONLY);

    //error handlings
    //file error
    if(fd == -1) {
        printf("unable to open file: %s\n", strerror(errno));
        exit(1);
    }

    ehdr = malloc(sizeof(Elf32_Ehdr));
    //malloc error
    if (!ehdr){
        printf("unable to perform malloc\n");
        exit(1);
    }
    //elf read error
    if (read(fd,ehdr,sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)){
        printf("unable to read elf header\n");
        exit(1);
    }

    //malloc error for prog header
    size_t phdr_size = sizeof(Elf32_Phdr)*ehdr->e_phnum;
    phdr = malloc(phdr_size);

    if(!phdr){
        printf("unable to do malloc for prog header\n");
        exit(1);
    }

    //lseek error
    if(lseek(fd, ehdr->e_phoff, SEEK_SET) == (off_t)-1){
        printf("error in lseek\n");
        exit(1);
    }

    //prog header read error
    if(read(fd, phdr, phdr_size) != (ssize_t)phdr_size){
        printf("unable to read program headers\n");
        exit(1);
    }

    //mem size calculation for pt load seg
    tot_pt_loadmem_size = 0;
    for(int i = 0; i < ehdr->e_phnum; i++){
        if(phdr[i].p_type == PT_LOAD){
            tot_pt_loadmem_size += phdr[i].p_memsz;
        }
    }

    //entry point address save and run it 
    entry_point = ehdr->e_entry;
    entry_func = (int (*)())entry_point;
    run_program(entry_func);
}


//run program _start()
void run_program(int (*_start)())
{
    int ret = _start();
    printf("program has returned: %d\n", ret);
}


// sigsegv handler,after page fault 
//this calls sunction handle page fault
void seg_v_handle(int signal, siginfo_t *info, void *context){
    (void)context;

    if(!info) {
        _exit(1);
    }

    uintptr_t fault_addr = (uintptr_t)info->si_addr;
    handle_page_fault(fault_addr);
}

//handles page loading , program will access unloaded page in memory
void handle_page_fault(uintptr_t addr)
{
    for(int i = 0; i < ehdr->e_phnum; i++) {

        if(phdr[i].p_type != PT_LOAD)
            continue;

        uintptr_t segment_start = phdr[i].p_vaddr;
        uintptr_t segment_end = segment_start + phdr[i].p_memsz;
        uintptr_t segment_page_end = (segment_end + PG_SIZE - 1) & ~(PG_SIZE - 1);
        
        //continue if fault address not present in segment 
        if(addr < segment_start || addr >= segment_page_end)
            continue;

        //page set
        uintptr_t page_start = addr & ~(PG_SIZE - 1);
        uintptr_t seg_page_zero = phdr[i].p_vaddr & ~(PG_SIZE - 1);
        off_t seg_file_off_zero = phdr[i].p_offset - (phdr[i].p_vaddr - seg_page_zero);

        
        uintptr_t seg_file_end = segment_start + phdr[i].p_filesz;
        uintptr_t page_end = page_start + PG_SIZE;

        //protection flags
        int prot = get_prot(phdr[i].p_flags);

        //alocation of new page 
        void *map = mmap((void *)page_start, PG_SIZE, prot | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,-1, 0);

        if(map == MAP_FAILED) {
            printf("unable to perform mmap: %s\n", strerror(errno));
            _exit(1);
        }

        uintptr_t fb_start;
        uintptr_t fb_end;
        size_t file_bytes;

        //copying bytes from file into page and caluclating it 
        if(page_start < segment_start) {
            fb_start = segment_start;
        }else {
            fb_start = page_start;
        }

        if(page_end <= seg_file_end) {
            fb_end = page_end;
        }else {
            fb_end = seg_file_end;
        }

        if(fb_end > fb_start) {
            file_bytes = fb_end - fb_start;
        }else {
            file_bytes = 0;
        }

        //reading elf file into the memory
        if(file_bytes > 0) {
            off_t file_offset = seg_file_off_zero + (fb_start - seg_page_zero);
            void *dest =(char *)map + (fb_start - page_start);
            ssize_t read_bytes = pread(fd, dest, file_bytes, file_offset);

            if(read_bytes < 0 || (size_t)read_bytes != file_bytes) {
                printf("unable to do pread: %s\n", strerror(errno));
                _exit(1);
            }
        }

        //making mem zero before file data
        if(fb_start > page_start) {
            memset(map, 0, fb_start - page_start);
        }

        //making file data zero ,bss
        uintptr_t tail_begin = (fb_end > page_start) ? fb_end : page_start;
        uintptr_t tail_end = (page_end <= segment_end) ? page_end : segment_end;

        if(tail_end > tail_begin) {
            memset((char *)map + (tail_begin - page_start), 0, tail_end - tail_begin);
        }

        //calculation updation
        total_pages++;
        page_faults++;
        return;
    }

    //again raising sigsegv if it is not a part of the load segment 
    signal(SIGSEGV, SIG_DFL);
    raise(SIGSEGV);
}

//making allocated memory free and closing fds
void Cleaner_loader()
{
    if(ehdr) {
        free(ehdr);
        ehdr = NULL;
    }
    if(phdr) {
        free(phdr);
        phdr = NULL;
    }
    if(fd != -1) {
        close(fd);
        fd = -1;
    }
}
