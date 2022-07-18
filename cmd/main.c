#include "mz.h"
#ifdef HAVE_COMPAT
#include "mz_compat.h"
#endif
#include "mz_crypt.h"
#include "mz_os.h"
#include "mz_strm.h"
#include "mz_strm_mem.h"
#include "mz_strm_os.h"
#ifdef HAVE_ZLIB
#include "mz_strm_zlib.h"
#endif
#include "mz_zip.h"

#include <stdio.h> /* printf, snprintf */

#if defined(_MSC_VER) && (_MSC_VER < 1900)
#  define snprintf _snprintf
#endif

#include <stdbool.h>

extern bool recurse_mkdir(const char *dirname);
extern unsigned char *read_file(const char *filename, size_t *len);
extern size_t write_file(const char *filename, const unsigned char *data, size_t len, bool append);

static int32_t test_unzip_compat_int(unzFile unzip)
{
    unz_global_info64 global_info64;
    unz_global_info global_info;
    unz_file_info64 file_info64;
    unz_file_info file_info;
    unz_file_pos file_pos;
    int32_t err = UNZ_OK;
    int32_t bytes_read = 0;
    char comment[120];
    char filename[120];
    char buffer[120];
    char *test_data = "test data";

    memset(&file_info, 0, sizeof(file_info));
    memset(&file_info64, 0, sizeof(file_info64));
    memset(&global_info, 0, sizeof(global_info));
    memset(&global_info64, 0, sizeof(global_info64));

    comment[0] = 0;
    err = unzGetGlobalComment(unzip, comment, sizeof(comment));
    if (err != UNZ_OK)
    {
        printf("Failed to get global comment (%" PRId32 ")\n", err);
        return err;
    }
    if (strcmp(comment, "test global comment") != 0)
    {
        printf("Unexpected global comment value (%s)\n", comment);
        return err;
    }
    err = unzGetGlobalInfo(unzip, &global_info);
    if (err != UNZ_OK)
    {
        printf("Failed to get global info  (%" PRId32 ")\n", err);
        return err;
    }
    err = unzGetGlobalInfo64(unzip, &global_info64);
    if (err != UNZ_OK)
    {
        printf("Failed to get global info 64-bit (%" PRId32 ")\n", err);
        return err;
    }
    if (global_info.number_entry != 2 || global_info64.number_entry != 2)
    {
        printf("Invalid number of entries in zip (%" PRId32 ")\n", global_info.number_entry);
        return err;
    }
    if (global_info.number_disk_with_CD != 0 || global_info64.number_disk_with_CD != 0)
    {
        printf("Invalid disk with cd (%" PRIu32 ")\n", global_info.number_disk_with_CD);
        return err;
    }

    err = unzLocateFile(unzip, "test.txt", (void *)1);
    if (err != UNZ_OK)
    {
        printf("Failed to locate test file (%" PRId32 ")\n", err);
        return err;
    }

    err = unzGoToFirstFile(unzip);
    if (err == UNZ_OK)
    {
        filename[0] = 0;
        err = unzGetCurrentFileInfo64(unzip, &file_info64, filename, sizeof(filename), NULL, 0, NULL, 0);
        if (err != UNZ_OK)
        {
            printf("Failed to get current file info 64-bit (%" PRId32 ")\n", err);
            return err;
        }

        err = unzOpenCurrentFile(unzip);
        if (err != UNZ_OK)
        {
            printf("Failed to open current file (%" PRId32 ")\n", err);
            return err;
        }
        bytes_read = unzReadCurrentFile(unzip, buffer, sizeof(buffer));
        if (bytes_read != (int32_t)strlen(test_data))
        {
            printf("Failed to read zip entry data (%" PRId32 ")\n", err);
            unzCloseCurrentFile(unzip);
            return err;
        }
        if (unzEndOfFile(unzip) != 1)
        {
            printf("End of unzip not reported correctly\n");
            return UNZ_INTERNALERROR;
        }
        err = unzCloseCurrentFile(unzip);
        if (err != UNZ_OK)
        {
            printf("Failed to close current file (%" PRId32 ")\n", err);
            return err;
        }

        if (unztell(unzip) != bytes_read)
        {
            printf("Unzip position not reported correctly\n");
            return UNZ_INTERNALERROR;
        }

        err = unzGoToNextFile(unzip);
        if (err != UNZ_OK)
        {
            printf("Failed to get next file info (%" PRId32 ")\n", err);
            return err;
        }

        comment[0] = 0;
        err = unzGetCurrentFileInfo(unzip, &file_info, filename, sizeof(filename), NULL, 0, comment, sizeof(comment));
        if (err != UNZ_OK)
        {
            printf("Failed to get current file info (%" PRId32 ")\n", err);
            return err;
        }
        if (strcmp(comment, "test local comment") != 0)
        {
            printf("Unexpected local comment value (%s)\n", comment);
            return err;
        }

        err = unzGetFilePos(unzip, &file_pos);
        if (err != UNZ_OK)
        {
            printf("Failed to get file position (%" PRId32 ")\n", err);
            return err;
        }
        if (file_pos.num_of_file != 1)
        {
            printf("Unzip file position not reported correctly\n");
            return UNZ_INTERNALERROR;
        }

        err = unzGetOffset(unzip);
        if (err <= 0)
        {
            printf("Unzip invalid offset reported\n");
            return UNZ_INTERNALERROR;
        }

        err = unzGoToNextFile(unzip);

        if (err != UNZ_END_OF_LIST_OF_FILE)
        {
            printf("Failed to reach end of zip entries (%" PRId32 ")\n", err);
            unzCloseCurrentFile(unzip);
            return err;
        }
        err = unzSeek64(unzip, 0, SEEK_SET);
    }

    return UNZ_OK;
}

int32_t test_unzip_compat(const char* zippath)
{
    unzFile unzip;
    int32_t err = UNZ_OK;

    unzip = unzOpen(zippath);
    if (unzip == NULL)
    {
        printf("Failed to open test zip file\n");
        return UNZ_PARAMERROR;
    }
    err = test_unzip_compat_int(unzip);
    unzClose(unzip);

    if (err != UNZ_OK)
        return err;

    printf("Compat unzip.. OK\n");

    return UNZ_OK;
}


#ifdef _WIN32
#  include <Windows.h>
#else
#  include <sys/stat.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "rw_files.h"
#include "zipper.h"

static const char *zipname = "test.zip";
static const char *f1_name = "Moneypenny.txt";
static const char *f2_name = "Bond.txt";
static const char *f3_name = "Up/M.txt";
static const char *f4_name = "Down/Q.tt";
static const char *f1_data = "secretary\n";
static const char *f2_data = "secret agent\n";
static const char *f3_data = "top guy\n";
static const char *f4_data = "bottom guy\n";
static const char *d1_name = "Up/";
static const char *d2_name = "Around/";
static const char *d3_name = "Bound/A/B";

static bool create_test_zip(void)
{
    zipFile zfile;

    zfile = zipOpen64(zipname, 0);
    if (zfile == NULL) {
        printf("Could not open %s for zipping\n", zipname);
        zipClose(zfile, NULL); 
        return false;
    }

    printf("adding dir: %s\n", d2_name);
    if (!zipper_add_dir(zfile, d2_name)) {
        printf("failed to write dir %s\n", d2_name);
        zipClose(zfile, NULL); 
        return false;
    }

    printf("adding dir: %s\n", d3_name);
    if (!zipper_add_dir(zfile, d3_name)) {
        printf("failed to write dir %s\n", d3_name);
        zipClose(zfile, NULL); 
        return false;
    }

    printf("adding file: %s\n", f1_name);
    if (!zipper_add_buf(zfile, f1_name, (const unsigned char *)f1_data, strlen(f1_data))) {
        printf("failed to write %s\n", f1_name);
        zipClose(zfile, NULL); 
        return false;
    }

    printf("adding file: %s\n", f2_name);
    if (!zipper_add_buf(zfile, f2_name, (const unsigned char *)f2_data, strlen(f2_data))) {
        printf("failed to write %s\n", f2_name);
        zipClose(zfile, NULL); 
        return false;
    }

    printf("adding file: %s\n", f3_name);
    if (!zipper_add_buf(zfile, f3_name, (const unsigned char *)f3_data, strlen(f3_data))) {
        printf("failed to write %s\n", f3_name);
        zipClose(zfile, NULL); 
        return false;
    }

    printf("adding dir: %s\n", d1_name);
    if (!zipper_add_dir(zfile, d1_name)) {
        printf("failed to write dir %s\n", d1_name);
        zipClose(zfile, NULL); 
        return false;
    }

    printf("adding file: %s\n", f4_name);
    if (!zipper_add_buf(zfile, f4_name, (const unsigned char *)f4_data, strlen(f4_data))) {
        printf("failed to write %s\n", f4_name);
        zipClose(zfile, NULL); 
        return false;
    }

    zipClose(zfile, NULL); 
    return true;
}

static bool unzip_test_zip(void)
{
    unzFile          uzfile;
    char            *zfilename;
    unsigned char   *buf;
    size_t           buflen;
    zipper_result_t  zipper_ret;
    uint64_t         len;

    uzfile = unzOpen64(zipname);
    if (uzfile == NULL) {
        printf("Could not open %s for unzipping\n", zipname);
        return false;
    }

    do {
        zipper_ret = ZIPPER_RESULT_SUCCESS;
        zfilename  = zipper_filename(uzfile, NULL);
        if (zfilename == NULL)
            return true;

        if (zipper_isdir(uzfile)) {
            printf("reading dir: %s\n", zfilename);
            recurse_mkdir(zfilename);
            unzGoToNextFile(uzfile);
            free(zfilename);
            continue;
        }

        len = zipper_filesize(uzfile);
        printf("reading file (%llu bytes): %s\n", len, zfilename);
        zipper_ret = zipper_read_buf(uzfile, &buf, &buflen);
        if (zipper_ret == ZIPPER_RESULT_ERROR) {
            free(zfilename);
            break;
        }

        recurse_mkdir(zfilename);
        write_file(zfilename, buf, buflen, false);
        free(buf);
        free(zfilename);
    } while (zipper_ret == ZIPPER_RESULT_SUCCESS);

    if (zipper_ret == ZIPPER_RESULT_ERROR) {
        printf("failed to read file\n");
        return false;
    }

    unzClose(uzfile);
    return true;
}

int main(int argc, char **argv)
{
    if (!create_test_zip())
        return 1;
    if (!unzip_test_zip())
        return 1;
    return 0;
}

// int main(int argc, const char *argv[])
// {
//     int32_t err = MZ_OK;
//     test_unzip_compat(argv[1]);
// }