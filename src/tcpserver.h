#ifndef TCP_SERVER
#define TCP_SERVER
#define _GNU_SOURCE
#include "tcpnet.h"

struct File {
    char* name;
    long bytes;
    uint8_t inUse;
    struct File* next;
    struct File* prev;
};

struct File* createFileNode(char* filename) {
    char* filePath = (char*)malloc(strlen(filename) + 7);
    if (filePath == NULL) { return NULL; }

    snprintf(filePath, strlen(filename) + 7, "%s/%s", "files", filename);

    struct File* newFile = (struct File*)malloc(sizeof(struct File));
    if (newFile == NULL) { free(filePath); return NULL; }
    
    // Use stat to get file size
    struct stat fileStat;
    if (!stat(filePath, &fileStat)) {
        newFile->bytes = (long)fileStat.st_size;
    } else {
        newFile->bytes = 0;
    }

    newFile->name = (char*)malloc(strlen(filename) + 1);
    if (newFile->name == NULL) {
        free(filePath);
        free(newFile);
        return NULL;
    }
    strcpy(newFile->name, filename);

    newFile->inUse = 0;
    newFile->next = NULL;
    newFile->prev = NULL;

    free(filePath);
    return newFile;
}

void insertFileNode(struct File** head, struct File* newFile) {
    if (*head == NULL) {
        *head = newFile;
        return;
    }

    struct File* p = *head;
    while (p->next != NULL) {
        p = p->next;
    }

    p->next = newFile;
    newFile->prev = p;
}

struct File* getFileNode(struct File* head, char* filename) {
    if (head == NULL) { return NULL; }

    while(strcmp(head->name, filename)) {
        head = head->next;
        if (head == NULL) { return NULL; }
    }

    return head;
}

void removeFileNode(struct File** head, struct File* file) {
    if (*head == NULL || file == NULL) { return; }

    if (*head == file) {
        *head = file->next;
    }

    if (file->prev != NULL) {
        file->prev->next = file->next;
    }

    if (file->next != NULL) {
        file->next->prev = file->prev;
    }

    free(file->name);
    free(file);
}

#endif
