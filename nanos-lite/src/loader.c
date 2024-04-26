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

extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);

static uintptr_t loader(PCB *pcb, const char *filename) {
    // Declare a variable of type Elf_Ehdr named ehdr to store the header information of the ELF file.
    Elf_Ehdr ehdr;
    // Use the ramdisk_read function to read the header information of the ELF file from the ramdisk,
    // starting from offset 0 and reading sizeof(Elf_Ehdr) bytes of data.
    ramdisk_read(&ehdr, 0, sizeof(Elf_Ehdr));
    // Check the validity of the ELF file
    assert(*(uint32_t*)(ehdr.e_ident) == 0x464C457FU);
    // Create an array of type Elf_Phdr named phdr to store the program header table information of the ELF file.
    // ehdr.e_phnum represents the number of header tables.
    Elf_Phdr phdr[ehdr.e_phnum];
    // Use the ramdisk_read function to read the program header table information from the ramdisk.
    // ehdr.e_ehsize indicates the offset of the program header table in the file,
    // sizeof(Elf_Phdr)*ehdr.e_phnum represents the number of bytes to read,
    // and all program header tables are read into the phdr array.
    ramdisk_read(phdr, ehdr.e_ehsize, sizeof(Elf_Phdr)*ehdr.e_phnum);
    for (int i = 0; i < ehdr.e_phnum; i++) {
        // Check if the type of the current program header table entry is PT_LOAD,
        // indicating that this is a segment that needs to be loaded into memory.
        if (phdr[i].p_type == PT_LOAD) {
            // Use the ramdisk_read function to read the content of the current segment from the ramdisk into memory.
            // phdr[i].p_vaddr represents the virtual address of the segment,
            // phdr[i].p_offset represents the offset of the segment in the file,
            // and phdr[i].p_memsz represents the size of the segment in memory.
            ramdisk_read((void*)phdr[i].p_vaddr, phdr[i].p_offset, phdr[i].p_memsz);
            // If the file size of the segment is smaller than the memory size,
            // this code is used to fill the uninitialized part (i.e., the .bss part) with zeros.
            memset((void*)(phdr[i].p_vaddr+phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
            }
    }
    // Return the entry address of the ELF file,
    // indicating the entry point of the program that is loaded and ready to execute.
    return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

