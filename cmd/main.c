#include "mz.h"
#include "mz_os.h"
#include "mz_strm.h"
#include "mz_strm_buf.h"
#include "mz_strm_split.h"
#include "mz_zip.h"
#include "mz_zip_rw.h"

#include <stdio.h>  /* printf */

typedef struct minizip_opt_s {
    uint8_t     include_path;
    int16_t     compress_level;
    uint8_t     compress_method;
    uint8_t     overwrite;
    uint8_t     append;
    int64_t     disk_size;
    uint8_t     follow_links;
    uint8_t     store_links;
    uint8_t     zip_cd;
    int32_t     encoding;
    uint8_t     verbose;
    uint8_t     aes;
    const char *cert_path;
    const char *cert_pwd;
} minizip_opt;
/***************************************************************************/

int32_t minizip_extract_entry_cb(void *handle, void *userdata, mz_zip_file *file_info, const char *path) {
    MZ_UNUSED(handle);
    MZ_UNUSED(userdata);
    MZ_UNUSED(path);

    /* Print the current entry extracting */
    // printf("Extracting %s\n", file_info->filename);
    return MZ_OK;
}

int32_t minizip_extract_progress_cb(void *handle, void *userdata, mz_zip_file *file_info, int64_t position) {
    minizip_opt *options = (minizip_opt *)userdata;
    double progress = 0;
    uint8_t raw = 0;

    MZ_UNUSED(userdata);

    mz_zip_reader_get_raw(handle, &raw);

    if (raw && file_info->compressed_size > 0)
        progress = ((double)position / file_info->compressed_size) * 100;
    else if (!raw && file_info->uncompressed_size > 0)
        progress = ((double)position / file_info->uncompressed_size) * 100;

    /* Print the progress of the current extraction */
    if (options->verbose)
        printf("%s - %" PRId64 " / %" PRId64 " (%.02f%%)\n", file_info->filename, position,
            file_info->uncompressed_size, progress);

    return MZ_OK;
}

int32_t minizip_extract_overwrite_cb(void *handle, void *userdata, mz_zip_file *file_info, const char *path) {
    // minizip_opt *options = (minizip_opt *)userdata;
    MZ_UNUSED(handle);
    MZ_UNUSED(file_info);
    return MZ_OK;
}

int32_t minizip_extract(const char *path, const char *pattern, const char *destination, const char *password, minizip_opt *options) {
    void *reader = NULL;
    int32_t err = MZ_OK;
    int32_t err_close = MZ_OK;

    // printf("Archive %s\n", path);

    /* Create zip reader */
    mz_zip_reader_create(&reader);
    mz_zip_reader_set_pattern(reader, pattern, 1);
    mz_zip_reader_set_password(reader, password);
    mz_zip_reader_set_encoding(reader, options->encoding);
    mz_zip_reader_set_entry_cb(reader, options, minizip_extract_entry_cb);
    mz_zip_reader_set_progress_cb(reader, options, minizip_extract_progress_cb);
    mz_zip_reader_set_overwrite_cb(reader, options, minizip_extract_overwrite_cb);

    err = mz_zip_reader_open_file(reader, path);

    if (err != MZ_OK) {
        printf("Error %" PRId32 " opening archive %s\n", err, path);
    } else {
        /* Save all entries in archive to destination directory */
        err = mz_zip_reader_save_all(reader, destination);

        if (err == MZ_END_OF_LIST) {
            if (pattern != NULL) {
                printf("Files matching %s not found in archive\n", pattern);
            } else {
                printf("No files in archive\n");
                err = MZ_OK;
            }
        } else if (err != MZ_OK) {
            printf("Error %" PRId32 " saving entries to disk %s\n", err, path);
        }
    }

    err_close = mz_zip_reader_close(reader);
    if (err_close != MZ_OK) {
        printf("Error %" PRId32 " closing archive for reading\n", err_close);
        err = err_close;
    }

    mz_zip_reader_delete(&reader);
    return err;
}

/***************************************************************************/
int32_t minizip_add_entry_cb(void *handle, void *userdata, mz_zip_file *file_info) {
    MZ_UNUSED(handle);
    MZ_UNUSED(userdata);
    MZ_UNUSED(file_info);

    /* Print the current file we are trying to compress */
    // printf("Adding %s\n", file_info->filename);
    return MZ_OK;
}

int32_t minizip_add_progress_cb(void *handle, void *userdata, mz_zip_file *file_info, int64_t position) {
    minizip_opt *options = (minizip_opt *)userdata;
    double progress = 0;
    uint8_t raw = 0;

    MZ_UNUSED(userdata);

    mz_zip_writer_get_raw(handle, &raw);

    if (raw && file_info->compressed_size > 0)
        progress = ((double)position / file_info->compressed_size) * 100;
    else if (!raw && file_info->uncompressed_size > 0)
        progress = ((double)position / file_info->uncompressed_size) * 100;

    /* Print the progress of the current compress operation */
    if (options->verbose)
        printf("%s - %" PRId64 " / %" PRId64 " (%.02f%%)\n", file_info->filename, position,
            file_info->uncompressed_size, progress);
    return MZ_OK;
}

int32_t minizip_add_overwrite_cb(void *handle, void *userdata, const char *path) {
    MZ_UNUSED(handle);
    MZ_UNUSED(userdata);
    MZ_UNUSED(path);
    return MZ_OK;
}

int32_t minizip_add(const char *path, const char *password, minizip_opt *options, int32_t arg_count, const char **args) {
    void *writer = NULL;
    int32_t err = MZ_OK;
    int32_t err_close = MZ_OK;
    int32_t i = 0;
    const char *filename_in_zip = NULL;


    printf("Archive %s\n", path);

    /* Create zip writer */
    mz_zip_writer_create(&writer);
    mz_zip_writer_set_password(writer, password);
    mz_zip_writer_set_aes(writer, options->aes);
    mz_zip_writer_set_compress_method(writer, options->compress_method);
    mz_zip_writer_set_compress_level(writer, options->compress_level);
    mz_zip_writer_set_follow_links(writer, options->follow_links);
    mz_zip_writer_set_store_links(writer, options->store_links);
    mz_zip_writer_set_overwrite_cb(writer, options, minizip_add_overwrite_cb);
    mz_zip_writer_set_progress_cb(writer, options, minizip_add_progress_cb);
    mz_zip_writer_set_entry_cb(writer, options, minizip_add_entry_cb);
    mz_zip_writer_set_zip_cd(writer, options->zip_cd);
    if (options->cert_path != NULL)
        mz_zip_writer_set_certificate(writer, options->cert_path, options->cert_pwd);

    err = mz_zip_writer_open_file(writer, path, options->disk_size, options->append);

    if (err == MZ_OK) {
        for (i = 0; i < arg_count; i += 1) {
            filename_in_zip = args[i];

            /* Add file system path to archive */
            err = mz_zip_writer_add_path(writer, filename_in_zip, NULL, options->include_path, 1);
            if (err != MZ_OK)
                printf("Error %" PRId32 " adding path to archive %s\n", err, filename_in_zip);
        }
    } else {
        printf("Error %" PRId32 " opening archive for writing\n", err);
    }

    err_close = mz_zip_writer_close(writer);
    if (err_close != MZ_OK) {
        printf("Error %" PRId32 " closing archive for writing %s\n", err_close, path);
        err = err_close;
    }

    mz_zip_writer_delete(&writer);
    return err;
}

int main(int argc, const char *argv[]) {
    const char* path = argv[1];
    const char* destination = argv[2];
    minizip_opt options;
    memset(&options, 0, sizeof(options));
    options.compress_method = MZ_COMPRESS_METHOD_DEFLATE;
    options.compress_level = MZ_COMPRESS_LEVEL_BEST; //MZ_COMPRESS_LEVEL_DEFAULT;

    int err = minizip_extract(path, NULL, destination, NULL, &options);
    options.include_path = 1;
    char* names[] = {"Payload", };
    minizip_add("wda.zip", NULL, &options, 1, names);
}
