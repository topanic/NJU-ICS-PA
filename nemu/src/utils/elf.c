//
// Created by czy on 4/4/24.
//

#include <elf.h>
#include "common.h"

/*
 * When we read all the information in the .elf file,
 * we can see these information below:
 *      Size of this header: 52 (bytes)
 *      Size of section headers: 40 (bytes)
 *      Start of section headers: 6108 (bytes into file)
 *
 * elf 文件头的大小是52个字节，所有的信息都包含在这52个字节里面，
 * 每一节表的长度是 40 字节, 这个貌似是确定的，（后面发现这个不是确定的，0x000002e 处给出了节头表每一节的长度， 所以这个并不确定。）
 * 52个字节里面 0x0000020 处给出了 section headers 的起始地址，40字节具体按照读取的elf文件中给出的位置来确定，这里为了方便只是举个例子）
 *
 * 在这之后就要去找 symtab 和 strtab 了，需要一个一个的比对，每个程序的的这两个表具体编号是几并不能确定
 *
 *
 * 关于如何判断函数是否为函数调用和函数返回指令：
 *      函数返回指令的特点是rd为x0，rs1为x1。
 *      函数跳转就只有两个 jal 和 jalr
 *
 * */

typedef struct {
    char name[64];
    paddr_t addr;      //the function head address
    Elf32_Xword size;
} Symbol;

Symbol *symbol = NULL;  //dynamic allocate memory  or direct allocate memory (Symbol symbol[NUM])

static size_t func_num = 0;

void load_elf_and_parse(const char *elf_file) {

    if (elf_file == NULL) return;

    FILE *fp;
    fp = fopen(elf_file, "rb");

    if (fp == NULL) {
        printf("failed to open the elf file!\n");
        exit(0);
    }

    Elf32_Ehdr edhr;
    //读取elf头
    if (fread(&edhr, sizeof(Elf32_Ehdr), 1, fp) <= 0) {
        printf("fail to read the elf_head!\n");
        exit(0);
    }

    if (edhr.e_ident[0] != 0x7f || edhr.e_ident[1] != 'E' ||
        edhr.e_ident[2] != 'L' || edhr.e_ident[3] != 'F') {
        printf("The opened file isn't a elf file!\n");
        exit(0);
    }

    fseek(fp, edhr.e_shoff, SEEK_SET);

    Elf32_Shdr shdr;
    char *string_table = NULL;
    //寻找字符串表
    for (int i = 0; i < edhr.e_shnum; i++) {
        if (fread(&shdr, sizeof(Elf32_Shdr), 1, fp) <= 0) {
            printf("fail to read the shdr\n");
            exit(0);
        }

        if (shdr.sh_type == SHT_STRTAB) {
            //获取字符串表
            string_table = malloc(shdr.sh_size);
            fseek(fp, shdr.sh_offset, SEEK_SET);
            if (fread(string_table, shdr.sh_size, 1, fp) <= 0) {
                printf("fail to read the strtab\n");
                exit(0);
            }
        }
    }

    //寻找符号表
    fseek(fp, edhr.e_shoff, SEEK_SET);

    for (int i = 0; i < edhr.e_shnum; i++) {
        if (fread(&shdr, sizeof(Elf32_Shdr), 1, fp) <= 0) {
            printf("fail to read the shdr\n");
            exit(0);
        }

        if (shdr.sh_type == SHT_SYMTAB) {
            fseek(fp, shdr.sh_offset, SEEK_SET);

            Elf32_Sym sym;

            size_t sym_count = shdr.sh_size / shdr.sh_entsize;
            symbol = malloc(sizeof(Symbol) * sym_count);

            for (size_t j = 0; j < sym_count; j++) {
                if (fread(&sym, sizeof(Elf32_Sym), 1, fp) <= 0) {
                    printf("fail to read the symtab\n");
                    exit(0);
                }

                if (ELF32_ST_TYPE(sym.st_info) == STT_FUNC) {
                    const char *name = string_table + sym.st_name;
                    strncpy(symbol[func_num].name, name, sizeof(symbol[func_num].name) - 1);
                    symbol[func_num].addr = sym.st_value;
                    symbol[func_num].size = sym.st_size;
                    func_num++;
                }
            }
        }
    }
//    Log("func_num: %d, ")
    fclose(fp);
    free(string_table);
}


static int rec_depth = 1;

void display_call_func(word_t pc, word_t func_addr) {
//    for(int i = 0; i <= func_num; i++) {
//        printf("%s\t0x%08x\t%lu\n", symbol[i].name, symbol[i].addr, symbol[i].size);
//    }
//    exit(0);

    int i = 0;
    word_t addr = 0;
    Elf32_Xword size = 0;
    for (; i < func_num; i++) {

        size = symbol[i].size;
        addr = symbol[i].addr;

        if (func_addr >= addr && func_addr < (size + addr)) {
            break;
        }
    }
    printf("0x%08x:", pc);

    for (int k = 0; k < rec_depth; k++) printf("  ");

    rec_depth++;

    printf("call  [%s@0x%08x]\n", symbol[i].name, func_addr);
}

void display_ret_func(word_t pc) {
    int i = 0;
    for (; i < func_num; i++) {
        if (pc >= symbol[i].addr && pc < (symbol[i].addr + symbol[i].size)) {
            break;
        }
    }
    printf("0x%08x:", pc);

    rec_depth--;

    for (int k = 0; k < rec_depth; k++) printf("  ");

    printf("ret  [%s]\n", symbol[i].name);
}
