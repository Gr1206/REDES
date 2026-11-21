#include <sys/stat.h>
#include <errno.h>
#include "inputValidation.h"

int checkUID(char* uid){
    int strsize = strlen(uid);
    if(strsize != UID_LEN) return 0; 

    for (int i = 0; i < strsize; i++){
        // Cast to unsigned to prevent undefined behaviour for bytes >= 128.
        if (!isdigit((unsigned char) uid[i]))
            return 0;
    }
    return 1; 
}

int checkPassword(char* password){
    int strsize = strlen(password);
    if(strsize != PWD_LEN) return 0;

    for (int i = 0; i < strsize; i++){
        // Cast to unsigned to prevent undefined behaviour for bytes >= 128.
        if (!isalnum((unsigned char) password[i]))
            return 0;
    }
    return 1; 
}

int checkPort(int port){
    if(port < PORT_MIN || port > PORT_MAX) return 0;
    return 1;
}

int checkChar(char c){
    // Cast to unsigned to prevent undefined behaviour for bytes >= 128.
    return isalnum((unsigned char) c) || c == '_' || c == '-';
}

int checkFilename(char* filename){
    int strsize = strlen(filename); 

    // nome.aaa extensão mais ponto final seriam 4 logo não pode haver fn menor que 5
    if (strsize > FILENAME_MAX_LEN || strsize < FILENAME_EXT_LEN + 2)
        return 0;

    if (filename[strsize - (FILENAME_EXT_LEN + 1)] != '.') {
        // printf("ponto não está no sítio %c\n", filename[strsize - 4]);
        return 0; //caso o ponto não esteja no sítio suposto
    }
    for (int char_index = 0; char_index < strsize - 4; char_index++){
        if(!checkChar(filename[char_index])) {
            //printf("Char inválido no base name %c\n", filename[char_index]); //mensagem para teste
            return 0;
        }   
    }

    for (int i = strsize - 3; i < strsize; i++){ //verificar extensão alfanumérica.
        if(!isalnum((unsigned char)filename[i])) {
            // printf("Char inválido na extensão %c\n", filename[i] );
            return 0;
        }
    }
    return 1;
}

int checkLabel(char* label){
    int labelLen = strlen(label);
    if(labelLen < LABEL_MIN_LEN || labelLen > LABEL_MAX_LEN) 
        return 0;

    for (int charIndex = 0; charIndex < labelLen; charIndex++){
        if(!checkChar(label[charIndex])){
            printf("Char inválido na label %c\n", label[charIndex]);
            return 0;
        }
    }
    return 1;
}

int checkFileSize(off_t size){
    if (size < 0 || size > (off_t)MAX_FILESIZE){
         return 0;
    }
    return 1;
}

int getLocalFileSize(const char *path, long *size) {
    struct stat fileInfo;

    if (stat(path, &fileInfo) != 0)
        return LOCAL_FILE_NOT_FOUND;
    if (!S_ISREG(fileInfo.st_mode))
        return LOCAL_FILE_NOT_REGULAR;
    if (!checkFileSize(fileInfo.st_size))
        return LOCAL_FILE_TOO_LARGE;

    *size = (long) fileInfo.st_size;
    return LOCAL_FILE_OK;
}

int checkPortStr(const char *s, int *port) {
    char *end;

    if (!isdigit((unsigned char) s[0]))
        return 0;

    errno = 0;
    long v = strtol(s, &end, 10);
    if (*end != '\0' || errno == ERANGE || v > PORT_MAX || !checkPort((int) v))
        return 0;

    *port = (int) v;
    return 1;
}