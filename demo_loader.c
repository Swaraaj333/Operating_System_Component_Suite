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
