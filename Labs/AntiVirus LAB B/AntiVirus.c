#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define VIRB "VIRB"
#define VIRL "VIRL"
#define BUFFER_SIZE 10000

typedef struct virus {
unsigned short SigSize;
char virusName[16];
unsigned char* sig;
} virus;

typedef struct link {
    struct link *nextVirus;
    virus *vir;
} link;


typedef struct fun_desc {
    char *name;
    void (*fun)();
} fun_desc;





bool isLittleEndian = true;
link* virus_list = NULL;  // Global list pointer





link* list_append(link* virus_list, virus* data); // Function declaration
void loadSignatures();
void printSignatures();
void detect_viruses();
void detect_virus(char *buffer, unsigned int size, link *virus_list);
void fixFile();
void quit(); // Rename from exit() to avoid conflict with stdlib
void list_free(link *virus_list);
void checkMagicNumber(FILE* file);
void printVirus(virus* virus, FILE* output);
link* list_append(link* virus_list, virus* data);
void list_print(link *virus_list, FILE* output);
virus* readVirus(FILE* file);

fun_desc menu[]= {
    { "Load signatures", loadSignatures },
    { "Print signatures", printSignatures },
    { "Detect viruses", detect_viruses },
    { "Fix file", fixFile },
    { "Quit", quit },
    { NULL, NULL }
};


//q.1.b
/// @brief Load signatures from a file


void loadSignatures() {
    char filename[100];
    FILE* fp;
    virus* v;
    
    printf("Enter signature file name: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = 0;
    
    fp = fopen(filename, "rb");
    if (!fp) {
        printf("Failed to open file\n");
        return;
    }

    char magic[4];
    fread(magic, sizeof(char), 4, fp);
    if (memcmp(magic, VIRL, 4) != 0) {
        printf("Invalid signature file\n");
        fclose(fp);
        return;
    }

    while ((v = readVirus(fp)) != NULL) {
        virus_list = list_append(virus_list, v);
    }
    
    fclose(fp);
}

//print signatures
void printSignatures() {
    if(virus_list == NULL) {
        printf("No signatures loaded\n");
        return;
    }
    list_print(virus_list, stdout);
}

//detect viruses right now not implemented
void detect_viruses() {
     char filename[100];
    FILE *fp;
    char buffer[BUFFER_SIZE];
    unsigned int size;

    // Get filename from user
    printf("Enter suspected file name: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = 0;

    // Open and read file
    fp = fopen(filename, "rb");
    if (!fp) {
        printf("Failed to open file\n");
        return;
    }

    // Read file content
    size = fread(buffer, 1, BUFFER_SIZE, fp);
    fclose(fp);

    if (size == 0) {
        printf("Error reading file or file is empty\n");
        return;
    }

    // Scan for viruses
    detect_virus(buffer, size, virus_list);
}


void detect_virus(char *buffer, unsigned int size, link *virus_list) {
    link *current = virus_list;
    while (current != NULL) {
        // For each byte in buffer up to (size - signature size)
        for (unsigned int i = 0; i <= size - current->vir->SigSize; i++) {
            // Compare bytes at current position with virus signature
            if (memcmp(buffer + i, current->vir->sig, current->vir->SigSize) == 0) {
                printf("Virus detected!\n");
                printf("Starting byte: %d\n", i);
                printf("Virus name: %s\n", current->vir->virusName);
                printf("Signature size: %d\n\n", current->vir->SigSize);
            }
        }
        current = current->nextVirus;
    }
}



//exit
void quit() {
    list_free(virus_list);
    exit(0);
}




//q.1.a
void checkMagicNumber(FILE* file) {
    char magic[5];
    if (fread(magic, 1, 4, file) != 4) {
        fprintf(stderr, "Error reading magic number\n");
        fclose(file);
        exit(1);
    }
    if (strcmp(magic, VIRB) == 0) {
        isLittleEndian = false;
    }
}

virus* readVirus(FILE* file) {
    virus* v = (virus*)malloc(sizeof(virus));
    if (isLittleEndian) {
        if (fread(&v->SigSize, 1, 2, file) != 2) {
            free(v);
            return NULL;
        }
    } 
    else {
        unsigned char buffer[2];
        if (fread(buffer, 1, 2, file) != 2) {
            free(v);
            return NULL;
        }
        v->SigSize = (buffer[0] << 8) | buffer[1]; // reconstruct 16-bit value
    }

    if (fread(v->virusName, 16, 1, file) != 1) {
        free(v);
        return NULL;
    }

    v->sig = (unsigned char*)malloc(v->SigSize);    

    if (fread(v->sig, 1, v->SigSize, file) != v->SigSize) {
        free(v->sig);
        free(v);
        return NULL;
    }

    return v;
}



void printVirus(virus* virus, FILE* output) {
    if (!virus || !output) {
        return;
    }

    
    fprintf(output, "Virus name: %s\n", virus->virusName);
    fprintf(output, "Virus size: %d\n", virus->SigSize);
    
   
    fprintf(output, "signature: ");
    for (int i = 0; i < virus->SigSize; i++) {
        fprintf(output, "%02X", virus->sig[i]);
        if (i < virus->SigSize - 1) {
            fprintf(output, " ");
        }
    }
    fprintf(output, "\n");
}

//q.1.a.2
void list_print(link *virus_list, FILE* output) {
    if (virus_list == NULL) {
        fprintf(output, "List is empty\n");
        return;
    }
    
    link *current = virus_list;
    while (current != NULL) {
        // Print virus name and signature size
        fprintf(output, "Virus name: %s\n", current->vir->virusName);
        fprintf(output, "Virus size: %d\n", current->vir->SigSize);
        
        // Print signature in hex format
        fprintf(output, "signature: ");
        for (int i = 0; i < current->vir->SigSize; i++) {
            fprintf(output, "%02X ", current->vir->sig[i]);
        }
        fprintf(output, "\n\n");  // Extra newline for separation
        
        current = current->nextVirus;
    }
}




link* list_append(link* virus_list, virus* data) {
    // Create new link node
    link* new_node = (link*)malloc(sizeof(link));
    if (new_node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    new_node->vir = data;
    new_node->nextVirus = NULL;

    // If list is empty, return new node
    if (virus_list == NULL) {
        return new_node;
    }

    // Add to beginning (prepend) - more efficient than traversing
    new_node->nextVirus = virus_list;
    return new_node;

}


void list_free(link *virus_list) {
    link *current = virus_list;
    link *next;
    
    while (current != NULL) {
        next = current->nextVirus;  // Store next pointer
        
        // Free virus data
        if (current->vir != NULL) {
            free(current->vir->sig);  // Free signature array
            free(current->vir);       // Free virus struct
        }
        
        free(current);  // Free link node
        current = next;
    }
}


void neutralize_virus(char *fileName, int signatureOffset) {
    FILE *fp = fopen(fileName, "r+b");  // Open for reading and writing
    if (!fp) {
        printf("Failed to open file for fixing\n");
        return;
    }

    // Seek to virus location
    if (fseek(fp, signatureOffset, SEEK_SET) != 0) {
        printf("Failed to seek to virus location\n");
        fclose(fp);
        return;
    }

    // Write RET instruction (0xC3)
    unsigned char ret = 0xC3;
    if (fwrite(&ret, sizeof(unsigned char), 1, fp) != 1) {
        printf("Failed to neutralize virus\n");
    }

    fclose(fp);
}

void fixFile() {
    char filename[100];
    FILE *fp;
    char buffer[BUFFER_SIZE];
    unsigned int size;

    // Get filename
    printf("Enter file to fix: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = 0;

    // Read file
    fp = fopen(filename, "rb");
    if (!fp) {
        printf("Failed to open file\n");
        return;
    }

    size = fread(buffer, 1, BUFFER_SIZE, fp);
    fclose(fp);

    if (size == 0) {
        printf("Error reading file\n");
        return;
    }

    // Detect and fix each virus
    link *current = virus_list;
    while (current != NULL) {
        for (unsigned int i = 0; i <= size - current->vir->SigSize; i++) {
            if (memcmp(buffer + i, current->vir->sig, current->vir->SigSize) == 0) {
                printf("Fixing virus '%s' at offset %d\n", current->vir->virusName, i);
                neutralize_virus(filename, i);
            }
        }
        current = current->nextVirus;
    }
}


int main(int argc, char *argv[]) {
     char buffer[256];
    int option;
    int bound = 0;

    // Count menu items
    while (menu[bound].name != NULL) {
        bound++;
    }

    while (1) {
        // Print menu
        printf("\nPlease choose a function:\n");
        for (int i = 0; i < bound; i++) {
            printf("%d) %s\n", i + 1, menu[i].name);
        }

        // Get user input
        if (fgets(buffer, 256, stdin) == NULL) {
            printf("Input error\n");
            continue;
        }

        // Parse input
        if (sscanf(buffer, "%d", &option) != 1) {
            printf("Invalid input\n");
            continue;
        }

        // Validate option
        if (option < 1 || option > bound) {
            printf("Not within bounds\n");
            continue;
        }

        // Execute selected function
        menu[option - 1].fun();
    }

return 0;
}