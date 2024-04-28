#include <proc.h>
#include <elf.h>
#include <fs.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

//extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
//extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);
//
//static uintptr_t loader(PCB *pcb, const char *filename) {
//    // Declare a variable of type Elf_Ehdr named ehdr to store the header information of the ELF file.
//    Elf_Ehdr ehdr;
//    // Use the ramdisk_read function to read the header information of the ELF file from the ramdisk,
//    // starting from offset 0 and reading sizeof(Elf_Ehdr) bytes of data.
//    ramdisk_read(&ehdr, 0, sizeof(Elf_Ehdr));
//    // Check the validity of the ELF file
//    assert(*(uint32_t*)(ehdr.e_ident) == 0x464C457FU);
//    // Create an array of type Elf_Phdr named phdr to store the program header table information of the ELF file.
//    // ehdr.e_phnum represents the number of header tables.
//    Elf_Phdr phdr[ehdr.e_phnum];
//    // Use the ramdisk_read function to read the program header table information from the ramdisk.
//    // ehdr.e_ehsize indicates the offset of the program header table in the file,
//    // sizeof(Elf_Phdr)*ehdr.e_phnum represents the number of bytes to read,
//    // and all program header tables are read into the phdr array.
//    ramdisk_read(phdr, ehdr.e_ehsize, sizeof(Elf_Phdr)*ehdr.e_phnum);
//    for (int i = 0; i < ehdr.e_phnum; i++) {
//        // Check if the type of the current program header table entry is PT_LOAD,
//        // indicating that this is a segment that needs to be loaded into memory.
//        if (phdr[i].p_type == PT_LOAD) {
//            // Use the ramdisk_read function to read the content of the current segment from the ramdisk into memory.
//            // phdr[i].p_vaddr represents the virtual address of the segment,
//            // phdr[i].p_offset represents the offset of the segment in the file,
//            // and phdr[i].p_memsz represents the size of the segment in memory.
//            ramdisk_read((void*)phdr[i].p_vaddr, phdr[i].p_offset, phdr[i].p_memsz);
//            // If the file size of the segment is smaller than the memory size,
//            // this code is used to fill the uninitialized part (i.e., the .bss part) with zeros.
//            memset((void*)(phdr[i].p_vaddr+phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
//            }
//    }
//    // Return the entry address of the ELF file,
//    // indicating the entry point of the program that is loaded and ready to execute.
//    return ehdr.e_entry;
//}

static void load_seg(int fd, uint32_t p_vaddr, uint32_t p_offset, uint32_t p_filesz, uint32_t p_memsz) {
    // 读取到内存指定位置
    void *addr = (void *) p_vaddr;
    size_t read_len;

    fs_lseek(fd, p_offset, SEEK_SET);
    read_len = fs_read(fd, addr, p_filesz);

    assert(read_len == p_filesz);
    memset((char *) p_vaddr + p_filesz, 0, p_memsz - p_filesz);
}

static uintptr_t loader(PCB *pcb, const char *filename) {
    assert(filename != NULL);
    int fd = fs_open(filename, 0, 0);

    Elf32_Ehdr elf_header;
    Elf32_Phdr pgm_header;
    // 读取elf header
    size_t read_len;
    // 从偏移量为0，读取长度为sizeof(Elf32_Ehdr)字节到elf_header中
    fs_lseek(fd, 0, SEEK_SET);
    read_len = fs_read(fd, (void *) (&elf_header), sizeof(Elf32_Ehdr));

    assert(read_len == sizeof(Elf32_Ehdr));

    // 魔数检查，前4字节是魔数，注意riscv为小端序
    assert(*(uint32_t *) (elf_header.e_ident) == 0x464C457FU);
    // 文件类型检查，应该是可执行文件
    assert(elf_header.e_type == ET_EXEC);
    // isa检查，应该是riscv
    assert(elf_header.e_machine == EM_RISCV);
    // elf version 检查，应该大于1
    assert(elf_header.e_version >= 1);

    // 将可以加载的seg从elf文件加载进内存的特定位置
    int i, off;
    for (i = 0, off = elf_header.e_phoff; i < elf_header.e_phnum; i++, off += sizeof(Elf32_Phdr)) {

        // 读取pro header信息
        fs_lseek(fd, off, SEEK_SET);
        read_len = fs_read(fd, (void *) (&pgm_header), sizeof(Elf32_Phdr));

        assert(read_len == sizeof(Elf32_Phdr));

        // 如果不能加载进入内存，继续
        if (pgm_header.p_type != PT_LOAD) {
            continue;
        }

        assert(pgm_header.p_memsz >= pgm_header.p_filesz);

        assert(pgm_header.p_memsz >= 0);

        // 加载入内存
        load_seg(fd, pgm_header.p_vaddr, pgm_header.p_offset, pgm_header.p_filesz, pgm_header.p_memsz);
    }

    fs_close(fd);
    return elf_header.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
    uintptr_t entry = loader(pcb, filename);
    Log("Jump to entry = %p", entry);
    ((void (*)()) entry)();
}

