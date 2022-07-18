#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#  include <Windows.h>
#else
#  include <sys/stat.h>
#endif

#ifdef _WIN32
const char SEP = '\\';
#else
const char SEP = '/';
#endif

bool recurse_mkdir(const char *dirname)
{
    const char *p;
    char       *temp;
    bool        ret = true;

    temp = calloc(1, strlen(dirname)+1);
    /* Skip Windows drive letter. */
#ifdef _WIN32
    if ((p = strchr(dirname, ':') != NULL) {
        p++;
    } else {
#endif
        p = dirname;
#ifdef _WIN32
    }
#endif

    while ((p = strchr(p, SEP)) != NULL) {
        /* Skip empty elements. Could be a Windows UNC path or
           just multiple separators which is okay. */
        if (p != dirname && *(p-1) == SEP) {
            p++;
            continue;
        }
        /* Put the path up to this point into a temporary to
           pass to the make directory function. */
        memcpy(temp, dirname, p-dirname);
        temp[p-dirname] = '\0';
        p++;
#ifdef _WIN32
        if (CreateDirectory(temp, NULL) == FALSE) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                ret = false;
                break;
            }
        }
#else
        if (mkdir(temp, 0774) != 0) {
            if (errno != EEXIST) {
                ret = false;
                break;
            }
        }
#endif
    }
    free(temp);
    return ret;
}

#include <stdlib.h>
#include <stdio.h>

#include <str_builder.h>

unsigned char *read_file(const char *filename, size_t *len)
{
    FILE          *f;
    str_builder_t *sb;
    char          *out;
    char           temp[8192];
    size_t         mylen;
    size_t         r;

    if (len == NULL)
        len = &mylen;
    *len = 0;

    if (filename == NULL || *filename == '\0' || len == NULL)
        return NULL;

    f = fopen(filename, "r");
    if (f == NULL)
        return NULL;
    
    sb = str_builder_create();
    do {
        r = fread(temp, sizeof(*temp), sizeof(temp), f);
        str_builder_add_str(sb, temp, r);
        *len += r;
    } while (r > 0);

    if (str_builder_len(sb) == 0) {
        fclose(f);
        str_builder_destroy(sb);
        return NULL;
    }
    fclose(f);

    out = str_builder_dump(sb, NULL);
    str_builder_destroy(sb);
    return (unsigned char *)out;
}

size_t write_file(const char *filename, const unsigned char *data, size_t len, bool append)
{
    FILE          *f;
    const char    *mode  = "w";
    size_t         wrote = 0;
    size_t         r;

    if (filename == NULL || *filename == '\0' || data == NULL)
        return 0;

    if (append)
        mode = "a";

    f = fopen(filename, mode);
    if (f == NULL)
        return 0;
    
    do {
        r = fwrite(data, sizeof(*data), len, f);
        wrote += r;
        len   -= r;
        data  += r;
    } while (r > 0 && len != 0);

    fclose(f);
    return wrote;
}