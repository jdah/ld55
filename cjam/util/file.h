#pragma once


#include "types.h"

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h> /* IWYU pragma: keep */

// convert relative file path with ~, /, etc. to absolute path with no fancy
// things
char *file_expand_path(allocator_t *al, const char *path);

// clean path
char *file_clean_path(allocator_t *al, const char *path);

// use format string to create cleaned path
char *file_pathfmt(allocator_t *al, const char *fmt, ...);

// join paths a and b
char *file_join_path(allocator_t *al, const char *a, const char *b);

// returns true if file exists
bool file_exists(const char *path);

// get file parent path, returns 0 on success
int file_parent(char *dst, usize n, const char *path);

// returns true if all directories leading up to file exists
bool file_parent_exists(const char *path);

// returns true if path is directory
bool file_isdir(const char *path);

// creates directory if it does not already exist
// if make_parents is true, then missing parent directories are also created
int file_mkdir(const char *path, bool make_parents);

// read entire file into buffer AND null terminate
int file_read_str(const char *path, char **pdata, usize *psz, allocator_t *al);

// read entire file into buffer
int file_read(const char *path, u8 **pdata, usize *psz, allocator_t *al);

// copy from from src to dst
int file_copy(const char *dst, const char *src);

// create backup of file at path with specified extension
// NOTE: THIS WILL OVERWRITE IF PATH ALREADY EXISTS WITH EX
int file_makebak(const char *path, const char *ext);

// write string data to file
int file_write_str(const char *path, const char *data);

// write bytes to file
int file_write(const char *path, const u8 *data, usize sz);

// find files with specified extension in path
// returns 0 on success
int file_find_with_ext(
    const char *dir, const char *ext, DYNLIST(char*) *paths, allocator_t *al);

// split file into base path, name, and extension (no ".")
// NOTE: modifies path in place!
void file_spilt(char *path, char **dir, char **name, char **ext);

// getcwd
char *file_cwd(allocator_t *al);

#ifdef UTIL_IMPL
#include "str.h"

#include <wordexp.h>

char *file_expand_path(allocator_t *al, const char *path) {
    wordexp_t exp;
    if (wordexp(path, &exp, 0)) {
        WARN("could not wordexp %s", path);
        return NULL;
    }

    char expanded[PATH_MAX];
    expanded[0] = '\0';

    for (int i = 0; i < (int) exp.we_wordc; i++) {
        xnprintf(
            expanded, PATH_MAX,
            "%s%s", exp.we_wordv[i], i == (int) exp.we_wordc - 1 ? "" : " ");
    }

    wordfree(&exp);

    /* char resolved[PATH_MAX]; */
    /* if (!realpath(expanded, resolved)) { */
    /*     WARN("could not realpath %s", expanded); */
    /*     return NULL; */
    /* } */

    /* const int resolved_len = strlen(resolved); */
    return mem_alloc_inplace(al, strlen(expanded) + 1, expanded);
}

char *file_clean_path(allocator_t *al, const char *path) {
    strbuf_t buf = strbuf_create(thread_scratch());
    
    bool have_sep = false;
    while (*path) {
       if (*path == '/') {
           if (have_sep) {
               // remove multiple separators in sequence
               goto nextch;
           }
    
           have_sep = true;
       } else {
            have_sep = false;
       }

       strbuf_ap_ch(&buf, *path);

nextch:
       path++;
    }

    return strbuf_dump(&buf, al);
}

char *file_pathfmt(allocator_t *al, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    const char *tmp = mem_vstrfmt(thread_scratch(), fmt, ap);
    char *out = file_clean_path(al, tmp);

    va_end(ap);

    return out;
}

char *file_join_path(allocator_t *al, const char *a, const char *b) {
    const char *p;
    usize len_a = strlen(a), sc_a = 0, sc_b = 0;

    if (len_a != 0) {
        p = &a[len_a - 1];
        while (p >= a && *p == '/') {
            sc_a++;
            p--;
        }
    }

    p = b;
    while (*p == '/') {
        sc_b++;
        p++;
    }

    return mem_strfmt(al, "%.*s/%s", max(len_a - sc_a, 0), &b[sc_b]);
}

bool file_exists(const char *path) {
    struct stat s;
    return !stat(path, &s);
}

int file_parent(char *dst, usize n, const char *path) {
    // TODO: bad
    const int res = snprintf(dst, n, "%s", path);
    if (res < 0 || res > (int) n) { return -1; }
    char *end = dst + strlen(dst) - 1;
    while (end != dst && *end != '/') {
        end--;
    }
    if (end == dst) { return -2; }
    *end = '\0';
    return 0;
}

bool file_parent_exists(const char *path) {
    char ppath[1024];
    if (!file_parent(ppath, sizeof(ppath), path)) { return false; }
    return file_exists(ppath);
}

bool file_isdir(const char *path) {
    struct stat s;
    if (stat(path, &s)) { return false; }
    return !S_ISREG(s.st_mode);
}

static int _mkdir(const char* path, mode_t mode) {
    if (!mkdir(path, mode)) {
        // OK
        return 0;
    }

    if (errno != EEXIST) {
        WARN("mkdir error: %s", strerror(errno));
        return -1;
    }

    struct stat st;
    if (stat(path, &st) != 0) {
        WARN("stat error: %s", strerror(errno));
        return -2;
    }

    // if not directory, fail
    if (!S_ISDIR(st.st_mode)) {
        WARN("%s exists but is not dir", path);
        return -1;
    }

    return 0;
}

int file_mkdir(const char *path, bool make_parents) {
    if (file_exists(path) && file_isdir(path)) {
        return 0;
    }

    int res;
    char tmp[PATH_MAX];
    char *p = NULL;

    const int len = snprintf(tmp, sizeof(tmp), "%s", path);
    if (len < 0 || len >= (int) sizeof(tmp)) { return -1; }

    if (tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    if (make_parents) {
        p = tmp + 1;

        while (*p) {
            if (*p == '/') {
                *p = 0;
                if ((res = _mkdir(tmp, S_IRWXU))) {
                    goto done;
                }
                *p = '/';
            }

            p++;
        }
    }

    res = _mkdir(tmp, 0777);

done:
    if (res) { WARN("mkdir error for path %s", path); }
    return res;
}

// INTERNAL USAGE ONLY
// read entire file into buffer, returns 0 on success
static int _file_read_internal(
    const char *path,
    bool is_str,
    u8 **pdata,
    usize *psz,
    allocator_t *al) {
    u8 *data;
    usize sz;

    FILE *f = fopen(path, "r");
    if (!f) { return -1; }

    fseek(f, 0, SEEK_END);
    const usize n_bytes = ftell(f);
    sz = n_bytes + (is_str ? 1 : 0);
    rewind(f);
    data = mem_alloc(al, sz);

    if (fread(data, n_bytes, 1, f) != 1) {
        fclose(f);
        mem_free(al, data);
        return -2;
    }

    fclose(f);

    if (is_str) {
        data[sz - 1] = '\0';
    }

    ASSERT(pdata);
    *pdata = data;
    if (psz) { *psz = sz; }

    return 0;
}

int file_read_str(const char *path, char **pdata, usize *psz, allocator_t *al) {
    return _file_read_internal(path, true, (u8**) pdata, psz, al);
}

int file_read(const char *path, u8 **pdata, usize *psz, allocator_t *al) {
    return _file_read_internal(path, false, pdata, psz, al);
}

int file_copy(const char *dst, const char *src) {
    u8 *data = NULL;
    FILE *fdst = NULL, *fsrc = NULL;
    int res = 0;

    fdst = fopen(dst, "wb");
    if (!fdst) { res = -1; goto done; }

    fsrc = fopen(src, "rb");
    if (!fsrc) { res = -2; goto done; }

    fseek(fsrc, 0, SEEK_END);
    const usize n_bytes = ftell(fsrc);
    rewind(fsrc);

    data = mem_alloc(g_mallocator, n_bytes);

    if (fread(data, n_bytes, 1, fsrc) != 1) { res = -3; goto done; }
    if (fwrite(data, n_bytes, 1, fdst) != 1) { res = -4; goto done; }

done:
    if (data) { mem_free(g_mallocator, data); }
    if (fdst) { fclose(fdst); }
    if (fsrc) { fclose(fdst); }
    return res;
}

int file_makebak(const char *path, const char *ext) {
    int res = 0;
    if (file_isdir(path)) { res = -1; goto done; }

    char new_path[1024];
    const int c = snprintf(new_path, sizeof(new_path), "%s%s", path, ext);
    if (c < 0 || c >= (int) sizeof(new_path)) {
        res = -2;
        goto done;
    }

    LOG("backing up %s -> %s", path, new_path);
    FILE *old_file = fopen(path, "rb"), *new_file = fopen(new_path, "wb");
    if (!old_file || !new_file) {
        WARN("got error %s", strerror(errno));
        res = -3;
        goto cleanup;
    }

    // TODO: horrible
    u8 byte;
    int n;
    while ((n = fread(&byte, sizeof(byte), 1, old_file))) {
        fwrite(&byte, sizeof(byte), 1, new_file);
    }

cleanup:
    if (old_file) { fclose(old_file); }
    if (new_file) { fclose(new_file); }

done:
    if (res != 0) { WARN("failed to make backup of %s", path); }
    return res;
}

int file_write_str(const char *path, const char *data) {
    FILE *f = fopen(path, "wb");
    if (!f) { return -1; }
    fprintf(f, "%s", data);
    fclose(f);
    return 0;
}

int file_write(const char *path, const u8 *data, usize sz) {
    FILE *f = fopen(path, "wb");
    if (!f) { WARN("%s", strerror(errno)); return -1; }
    if (fwrite(data, sz, 1, f) != 1) { return -2; }
    fclose(f);
    return 0;
}

int file_find_with_ext(
    const char *dir, const char *ext, DYNLIST(char*) *paths, allocator_t *al) {
    DIR *d = opendir(dir);
    if (!d) { return -1; }

    char buf[1024], suf[64];
    snprintf(suf, sizeof(suf), ".%s", ext);

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_type != DT_REG) { continue; }
        if (strsuf(ent->d_name, suf)) { continue; }

        // make full path
        snprintf(buf, sizeof(buf), "%s/%s", dir, ent->d_name);

        const int len = strlen(buf);
        char *p = al->alloc(al, len + 1);

        snprintf(p, len + 1, "%s", buf);
        *dynlist_push(*paths) = p;
    }

    closedir(d);
    return 0;
}

void file_spilt(char *path, char **pdir, char **pname, char **pext) {
    char *dir = NULL, *name = NULL, *ext = NULL;

    char *last_sep = strrchr(path, '/');
    char *dot = strrchr(path, '.');

    if (dot && last_sep) {
        *last_sep = '\0';
        dir = path;
        name = last_sep + 1;

        // only extension if occurs after last separator
        if (dot > last_sep) {
            *dot = '\0';
            ext = dot + 1;
        }
    } else if (!dot && last_sep) {
        *last_sep = '\0';
        dir = path;
        name = last_sep + 1;
    } else if (dot && !last_sep) {
        *dot = '\0';
        name = path;
        ext = dot + 1;
    } else {
        // nothing else other than name
        name = path;
    }

    if (pdir) { *pdir = dir; }
    if (pname) { *pname = name; }
    if (pext) { *pext = ext; }
}

char *file_cwd(allocator_t *al) {
    char buf[PATH_MAX];
    getcwd(buf, sizeof(buf));
    return mem_strdup(al, buf);
}
#endif // ifdef UTIL_IMPL
