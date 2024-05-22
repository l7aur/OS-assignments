#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>

#define WRITE_PIPE_NAME "RESP_PIPE_37962"
#define REQ_PIPE_NAME "REQ_PIPE_37962"

const char CONNECT_TO_PIPE[] = {strlen("CONNECT"), 'C', 'O', 'N', 'N', 'E', 'C', 'T', '\0'};

int fd_read, fd_write, fd;
char * data;
char * file_data;
unsigned long file_size;

int start() {
    if(mkfifo(WRITE_PIPE_NAME, 0600) < 0) return -4;
    if((fd_read = open(REQ_PIPE_NAME, O_RDONLY)) < 0)  return -3;
    if((fd_write = open(WRITE_PIPE_NAME, O_WRONLY)) < 0) return -2;
    if(write(fd_write, CONNECT_TO_PIPE, sizeof(CONNECT_TO_PIPE) - 1) < 0) return -1;
    return 0;
}
void end() {
    //munmap(data, 0);
    close(fd_read);
    close(fd_write);
    close(fd);
    shm_unlink("/U7yOLa");
    unlink(REQ_PIPE_NAME);
    unlink(WRITE_PIPE_NAME);
}

void handleECHO() {
    char s[] = {strlen("ECHO"), 'E', 'C', 'H', 'O', '\0'};
    char p[] = {strlen("VARIANT"), 'V', 'A', 'R', 'I', 'A', 'N', 'T', '\0'};
    char q[] = {0x4A, 0x94, 0x00, 0x00};
    write(fd_write, s, sizeof(s) - 1);
    write(fd_write, p, sizeof(p) - 1);
    write(fd_write, q, sizeof(q));
}

int power(int base, int exp) {
    if(exp < 0)
        return -1;
    int rez = 1;
    while(exp) {
        rez *= base;
        exp--;
    }
    return rez;
}

void print_success() {
    const char m1[] = {strlen("SUCCESS"), 'S', 'U', 'C', 'C', 'E', 'S', 'S', '\0'};
    write(fd_write, m1, sizeof(m1) - 1);
}

void print_error() {
    const char m2[] = {strlen("ERROR"), 'E', 'R', 'R', 'O', 'R', '\0'};
    write(fd_write, m2, sizeof(m2) - 1);
}

unsigned int getNumber() {
    char n[10];
    int sizee = read(fd_read, n, sizeof(unsigned int));
    n[sizee] = '\0';
    unsigned int rez = 0;
    for(int i = 0; i < sizeof(unsigned int); i++) {
        unsigned int d0 = (unsigned int)(n[i] & 0x0F);
        unsigned int d1 = (unsigned int)(n[i] & 0xF0) >> 4;
        rez += (d0 + d1 * 16) * power(16, 2 * i);
    }
    return rez;
}

void handleCSHM(){
    unsigned int rez = getNumber();
    
    char s[] = {strlen("CREATE_SHM"), 'C', 'R', 'E', 'A', 'T', 'E', '_', 'S', 'H', 'M', '\0'};
    write(fd_write, s, sizeof(s) - 1);

    fd = shm_open("/U7yOLa", O_CREAT | O_RDWR, 0644);
    if (fd >= 0 && ftruncate(fd, sizeof(char) * rez) == 0) {
        data = (char *)mmap(0, sizeof(char) * rez, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if(data == MAP_FAILED) 
            print_error(); 
        else 
            print_success();
    }
    else 
        print_error();
}

int noBytes(unsigned int n) {
    if(n == 0)
        return 1;
    int x = 0;
    while(n != 0) {
        x++;
        n /= 16;
    }
    return (x + 1) / 2;
}

void handleWSHM() {
    unsigned int n1 = getNumber();
    unsigned int n2 = getNumber();
    int sizeofn2 = noBytes(n2);
    char s[] = {strlen("WRITE_TO_SHM"), 'W', 'R', 'I', 'T', 'E','_','T', 'O', '_', 'S', 'H', 'M', '\0'};
    write(fd_write, s, sizeof(s) - 1);
    if(n1 < 0) {
        print_error();
        return;
    }
    if(n1 + sizeofn2 > 3332442) {
        print_error();
        return;
    }
    for(int i = 0; i < sizeofn2; i++){
        (*(data + n1 + i)) = (char)(n2 % 256);
        n2 /= 256;
    }
    print_success();
}

void handleMAP() {
    int sizee = 0;
    char addr[300];
    read(fd_read, &sizee, 1);
    int n = read(fd_read, addr, sizee);
    addr[n] = '\0';

    char s[] = {strlen("MAP_FILE"), 'M', 'A', 'P','_','F', 'I', 'L', 'E', '\0'};
    write(fd_write, s, sizeof(s) - 1);

    int fd_file;

    if((fd_file = open(addr, O_RDONLY, 0644)) < 0) {
        print_error();
        return;
    }
    struct stat st;
    if(stat(addr, &st) < 0) {
        print_error();
        return;
    }
    file_size = st.st_size;
    file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd_file, 0);
    if(fd_file < 0)
        print_error();
    else
        print_success();
    close(fd_file);
}

void handleRFFO() {
    unsigned int n1 = getNumber();
    unsigned int n2 = getNumber();

    char s[] = {strlen("READ_FROM_FILE_OFFSET"), 'R', 'E', 'A', 'D', '_','F', 'R', 'O', 'M',
    '_', 'F', 'I', 'L', 'E', '_', 'O', 'F', 'F', 'S', 'E', 'T', '\0'};
    write(fd_write, s, sizeof(s) - 1);
    
    if(file_size < n1 + n2 ) {
        print_error();
        return;
    }
    memcpy(data, (char*)file_data + n1, n2);
    print_success();
}

void handleRFFSR() {
    unsigned int section_no = getNumber();
    unsigned int offset = getNumber();
    unsigned int no_bytes = getNumber();
    
    char s[] = {strlen("READ_FROM_FILE_SECTION"), 'R', 'E', 'A', 'D', '_','F', 'R', 'O', 'M',
    '_', 'F', 'I', 'L', 'E', '_', 'S', 'E', 'C', 'T', 'I', 'O', 'N', '\0'};
    write(fd_write, s, sizeof(s) - 1);

    int ch1 = file_data[file_size - 2];
    int ch2 = file_data[file_size - 3];
    
    if(ch1 < 0) ch1 += 256;
    if(ch2 < 0) ch2 += 256;
    
    unsigned int header_size = (ch1 << 8) + ch2;
    unsigned int index = file_size - header_size + 2;
    unsigned int no_of_sections = file_data[index++];

    if(no_of_sections < section_no) {
        print_error();
        return;
    }
    index += (19 + 3 * 4) * (section_no - 1) + 19 + 4;

    ch1 = file_data[index++];
    ch2 = file_data[index++];
    int ch3 = file_data[index++];
    int ch4 = file_data[index++];
    if(ch1 < 0) ch1 += 256;
    if(ch2 < 0) ch2 += 256;
    if(ch3 < 0) ch3 += 256;
    if(ch4 < 0) ch4 += 256;

    unsigned int section_offset = (ch4 << 24) + (ch3 << 16) +
                                  (ch2 << 8) + ch1;
    ch1 = file_data[index++];
    ch2 = file_data[index++];
    ch3 = file_data[index++];
    ch4 = file_data[index++];
    if(ch1 < 0) ch1 += 256;
    if(ch2 < 0) ch2 += 256;
    if(ch3 < 0) ch3 += 256;
    if(ch4 < 0) ch4 += 256;

    unsigned int section_size = (ch4 << 24) + (ch3 << 16) + (ch2 << 8) + ch1;

    if(section_size < offset + no_bytes) {
        print_error();
        return;
    }

    memcpy(data, file_data + section_offset + offset, no_bytes);
    print_success();
}

#define SIZE_OF_SECTION_HEADER 31

void handleRFLSO() {
    int log_offset = getNumber();
    unsigned int no_bytes = getNumber();
    char s[] = {strlen("READ_FROM_LOGICAL_SPACE_OFFSET"), 'R', 'E', 'A', 'D', '_','F', 'R', 'O', 'M',
    '_', 'L', 'O', 'G', 'I', 'C', 'A', 'L', '_', 'S', 'P', 'A', 'C', 'E', '_', 'O', 'F', 'F', 'S', 'E', 'T', '\0'};
    write(fd_write, s, sizeof(s) - 1);

    int ch1 = file_data[file_size - 2];
    int ch2 = file_data[file_size - 3];
    
    if(ch1 < 0) ch1 += 256;
    if(ch2 < 0) ch2 += 256;
    
    unsigned int header_size = (ch1 << 8) + ch2;
    unsigned int index = file_size - header_size + 3;
    ch1 = file_data[index - 1];
    if(ch1 < 0) ch1 += 256;
    unsigned int no_sections = ch1;
    
    char section_header_string[35];
    unsigned int section_size = 0;
    for(int i = 0; i < no_sections; i++)
    {
        memcpy(section_header_string, file_data + index, SIZE_OF_SECTION_HEADER);
        index += SIZE_OF_SECTION_HEADER;
        ch1 = section_header_string[27]; 
        ch2 = section_header_string[28];
        int ch3 = section_header_string[29]; 
        int ch4 = section_header_string[30];

        if(ch1 < 0) ch1 += 256;
        if(ch2 < 0) ch2 += 256;
        if(ch3 < 0) ch3 += 256;
        if(ch4 < 0) ch4 += 256;
        section_size = (ch4 << 24) + (ch3 << 16) + (ch2 << 8) + ch1;
        int add_on = 0;
        if(section_size % 2048) add_on = 1;
        if(log_offset < 2048 * (section_size / 2048 + add_on))
            break;
        log_offset -= 2048 * (section_size / 2048 + add_on);
    }

    ch1 = section_header_string[23];
    ch2 = section_header_string[24];
    int ch3 = section_header_string[25];
    int ch4 = section_header_string[26];
    if(ch1 < 0) ch1 += 256;
    if(ch2 < 0) ch2 += 256;
    if(ch3 < 0) ch3 += 256;
    if(ch4 < 0) ch4 += 256;
    unsigned int offset = (ch4 << 24) + (ch3 << 16) + (ch2 << 8) + ch1;

    if(log_offset + no_bytes > section_size) {
        print_error();
        return;
    }
    memcpy(data, file_data + offset + log_offset, no_bytes);
    print_success();
}

int main() {
    switch(start()) {
        case -4:
            printf("ERROR\ncannot create the response pipe\n");
            exit(-1);
        case -3:
            printf("ERROR\ncannot open the request pipe\n");
            exit(-1);
        case -2:
            printf("je-CANNOT OPEN PIPE FOR WRITING\n");
            exit(-1);
        case -1:
            printf("je-CANNOT WRITE ON PIPE\n");
            exit(-1);
        case 0:
            printf("SUCCESS\n");
            break;
        default:
            printf("WHY ARE YOU HERE?\n");
            exit(-1);
    }

    char * buffer = (char *)malloc(300 * sizeof(char));
    
    while(1) {
        //get command from pipe
        int n = 0;
        buffer[0] = '\0';
        int how_much = read(fd_read, buffer, 1);
        how_much = (int)buffer[0];
        n = read(fd_read, buffer, how_much);
        buffer[n] = '\0';

        if(!strncmp(buffer, "EXIT", strlen("EXIT")))
            break;
        else if(!strncmp(buffer, "ECHO", strlen("ECHO")))
            handleECHO();
        else if(!strncmp(buffer, "CREATE_SHM", strlen("CREATE_SHM")))
            handleCSHM();
        else if(!strncmp(buffer, "WRITE_TO_SHM", strlen("WRITE_TO_SHM")))
            handleWSHM();
        else if(!strncmp(buffer, "MAP_FILE", strlen("MAP_FILE")))
            handleMAP();
        else if(!strncmp(buffer, "READ_FROM_FILE_OFFSET", strlen("READ_FROM_FILE_OFFSET")))
            handleRFFO();
        else if(!strncmp(buffer, "READ_FROM_FILE_SECTION", strlen("READ_FROM_FILE_SECTION")))
           handleRFFSR();
        else if(!strncmp(buffer, "READ_FROM_LOGICAL_SPACE_OFFSET", strlen("READ_FROM_LOGICAL_SPACE_OFFSET")))
           handleRFLSO();
        else
            exit(-1);
    }

    free(buffer);
    end();
    return 0;
}