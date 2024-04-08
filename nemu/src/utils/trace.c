


#include <common.h>
#include <elf.h>

#define INST_NUM 16

#ifdef IRINGBUF
/********************************** iringbuf *********************************************/
typedef struct
{
    word_t pc;
    uint32_t inst;
}InstBuf;

InstBuf iringbuf[INST_NUM];

int cur_inst = 0;

void trace_inst(word_t pc, uint32_t inst)
{
    iringbuf[cur_inst].pc = pc;
    iringbuf[cur_inst].inst = inst;
    cur_inst = (cur_inst + 1) % INST_NUM;
}

void display_inst()
{
    /*** 注意出错的是前一条指令，当前指令可能由于出错已经无法正常译码 ***/
    int end = cur_inst;
    char buf[128];
    char *p;
    int i = cur_inst;

    if(iringbuf[i+1].pc == 0) i = 0;

    printf("----------------------------iringbuf----------------------------\n");

    do
    {
        p = buf;
        //if(i == end) p += sprintf(buf, "-->");
        p += sprintf(buf, "%s" FMT_WORD ":  %08x\t", (i + 1) % INST_NUM == end ? "-->" : "   ", iringbuf[i].pc, iringbuf[i].inst);

        void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
        disassemble(p, buf + sizeof(buf) - p, iringbuf[i].pc, (uint8_t *)&iringbuf[i].inst, 4);

        puts(buf);
        i = (i + 1) % INST_NUM;
    } while (i != end);

    printf("----------------------------iringbuf----------------------------\n\n");

}
#endif


#ifdef CONFIG_FTRACE
/************************************* ftrace ********************************************/

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

#endif

#ifdef CONFIG_DTRACE
// dtrace
void display_device_read(paddr_t addr, int len, IOMap *map) {
    log_write(ANSI_FMT("read memory: ", ANSI_FG_BLUE) FMT_PADDR ", the len is %d, the read device is "
                    ANSI_FMT(" %s ", ANSI_BG_BLUE) "\n", addr, len, map->name);
}

void display_device_write(paddr_t addr, int len, word_t data, IOMap *map) {
    log_write(ANSI_FMT("write memory: ", ANSI_FG_YELLOW) FMT_PADDR ", the len is %d, the written data is " FMT_WORD
                    ", the written device is "ANSI_FMT(" %s ", ANSI_BG_YELLOW)"\n", addr, len, data, map->name);
}
#endif