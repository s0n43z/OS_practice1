#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

typedef void (*set_key_func)(char);
typedef void (*caesar_func)(void*, void*, int);

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <libpath> <key> <input file> <output file>\n", argv[0]);
        return 1;
    }

    const char *libpath = argv[1];
    char key = (char)atoi(argv[2]); // поддержка числового ключа
    const char *input_path = argv[3];
    const char *output_path = argv[4];

    void *handle = dlopen(libpath, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error loading library: %s\n", dlerror());
        return 1;
    }

    set_key_func set_key = (set_key_func)dlsym(handle, "set_key");
    caesar_func caesar = (caesar_func)dlsym(handle, "caesar");

    if (!set_key || !caesar) {
        fprintf(stderr, "Error loading symbols: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

    FILE *in = fopen(input_path, "rb");
    if (!in) {
        perror("Failed to open input file");
        dlclose(handle);
        return 1;
    }

    fseek(in, 0, SEEK_END);
    long len = ftell(in);
    fseek(in, 0, SEEK_SET);

    unsigned char *buffer = (unsigned char*)malloc(len);
    if (!buffer) {
        perror("Memory allocation failed");
        fclose(in);
        dlclose(handle);
        return 1;
    }

    fread(buffer, 1, len, in);
    fclose(in);

    set_key(key);
    caesar(buffer, buffer, len);

    FILE *out = fopen(output_path, "wb");
    if (!out) {
        perror("Failed to open output file");
        free(buffer);
        dlclose(handle);
        return 1;
    }

    fwrite(buffer, 1, len, out);
    fclose(out);

    printf("Success: processed %ld bytes with key %d\n", len, (int)key);

    free(buffer);
    dlclose(handle);
    return 0;
}