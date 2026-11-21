/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of PvZ-Portable.
 *
 * PvZ-Portable is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PvZ-Portable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PvZ-Portable. If not, see <https://www.gnu.org/licenses/>.
 */

#include "fcaseopen.h"

#if defined(_WIN32)
#include <direct.h> // chdir on Windows
#include <malloc.h> // alloca on Windows (MSYS2/UCRT)
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#else
#include <unistd.h> // fix "implicit declaration of function chdir"
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#endif
#ifdef __HAIKU__
// this function seems to not exist under haiku??
char *strsep(char **stringp, const char *delim)
{
	char *begin, *end;
	begin = *stringp;
	if (begin == NULL) return NULL;

	if (delim[0] == '\0' || begin[0] == '\0')
	{
		*stringp = NULL;
		return begin;
	}

	end = strpbrk(begin, delim);
	if (end)
	{
		*end = '\0';
		*stringp = end + 1;
	}
	else
	{
		*stringp = NULL;
	}
	return begin;
}
#endif

#if !defined(_WIN32)
// r must have strlen(path) + 3 bytes
int casepath(char const *path, char *r)
{
    size_t l = strlen(path);
    char *p = (char *)alloca(l + 1);
    strcpy(p, path);
    size_t rl = 0;
    
    DIR *d;
    if (p[0] == '/')
    {
        d = opendir("/");
        p = p + 1;
    }
    else
    {
        d = opendir(".");
        r[0] = '.';
        r[1] = 0;
        rl = 1;
    }
    
    int last = 0;
    char *c = strsep(&p, "/");
    while (c)
    {
        if (!d)
        {
            return 0;
        }
        
        if (last)
        {
            closedir(d);
            return 0;
        }
        
        r[rl] = '/';
        rl += 1;
        r[rl] = 0;
        
        struct dirent *e = readdir(d);
        while (e)
        {
            if (strcasecmp(c, e->d_name) == 0)
            {
                strcpy(r + rl, e->d_name);
                rl += strlen(e->d_name);

                closedir(d);
                d = opendir(r);
                
                break;
            }
            
            e = readdir(d);
        }
        
        if (!e)
        {
            strcpy(r + rl, c);
            rl += strlen(c);
            last = 1;
        }
        
        c = strsep(&p, "/");
    }
    
    if (d) closedir(d);
    return 1;
}

// r must have strlen(base) + strlen(path) + 3 bytes
int casepathat(char const *base, char const *path, char *r)
{
    if (!base || base[0] == '\0')
        base = ".";

    // Copy base directly - it's already case-correct
    strcpy(r, base);
    size_t rl = strlen(r);
    if (rl == 0)
    {
        r[0] = '.';
        r[1] = 0;
        rl = 1;
    }

    // Ensure trailing separator
    if (r[rl - 1] != '/' && r[rl - 1] != '\\')
    {
        r[rl++] = '/';
        r[rl] = 0;
    }

    // Open base directory directly (no case correction needed)
    DIR *d = opendir(base);
    if (!d)
        return 0;

    // Prepare to parse the relative path
    char *p = (char *)alloca(strlen(path) + 1);
    strcpy(p, path);

    int last = 0;
    char *c = strsep(&p, "/\\");
    while (c)
    {
        if (!d)
            return 0;

        if (last)
        {
            closedir(d);
            return 0;
        }

        // Skip empty components (e.g., from leading or double slashes)
        if (c[0] == 0)
        {
            c = strsep(&p, "/\\");
            continue;
        }

        r[rl] = '/';
        rl += 1;
        r[rl] = 0;

        struct dirent *e = readdir(d);
        while (e)
        {
            if (strcasecmp(c, e->d_name) == 0)
            {
                strcpy(r + rl, e->d_name);
                rl += strlen(e->d_name);

                closedir(d);
                d = opendir(r);
                break;
            }
            e = readdir(d);
        }

        if (!e)
        {
            strcpy(r + rl, c);
            rl += strlen(c);
            last = 1;
        }

        c = strsep(&p, "/\\");
    }

    if (d) closedir(d);
    return 1;
}
#endif

#if defined(_WIN32)
static wchar_t *utf8_to_wide_alloc(const char *utf8)
{
	if (!utf8)
		return NULL;
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	if (len <= 0)
		return NULL;
	wchar_t *wide = (wchar_t *)malloc(sizeof(wchar_t) * (size_t)len);
	if (!wide)
		return NULL;
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, len);
	return wide;
}
#endif

#if !defined(_WIN32)
// switchfish: directory indexes. The resource code probes every file under several extensions, and each
// miss used to scan the whole directory (427 entries in images/): on the Switch's SD card that was most of
// a 38 s boot. Now each directory is read once; a read-mode open looks the name up in memory and opens the
// real spelling, and a name the index doesn't have is missing without touching the SD card (each failed
// fopen there costs milliseconds, and there are ~1900 per boot). Everything that changes a directory calls
// fcaseopen_forget (Sexy::CommitTempFile for saves and settings, DeleteFile, MkDir); write-mode opens
// through here drop the indexes too.
typedef struct
{
    char *dir;
    char **names;
    int count;
} DirIndex;

#define MAX_DIR_INDEXES 64
static DirIndex gDirIndexes[MAX_DIR_INDEXES];
static int gDirIndexCount = 0;
static pthread_mutex_t gDirIndexLock = PTHREAD_MUTEX_INITIALIZER;
static int gStatOpens = 0, gStatMisses = 0, gStatScans = 0;

static void drop_dir_indexes(void)
{
    for (int i = 0; i < gDirIndexCount; i++)
    {
        for (int j = 0; j < gDirIndexes[i].count; j++)
            free(gDirIndexes[i].names[j]);
        free(gDirIndexes[i].names);
        free(gDirIndexes[i].dir);
    }
    gDirIndexCount = 0;
}

static DirIndex *dir_index(const char *dir)
{
    for (int i = 0; i < gDirIndexCount; i++)
    {
        if (strcmp(gDirIndexes[i].dir, dir) == 0)
            return &gDirIndexes[i];
    }
    if (gDirIndexCount == MAX_DIR_INDEXES)
        return NULL;
    DIR *d = opendir(dir);
    if (!d)
        return NULL;
    gStatScans++;
    DirIndex *x = &gDirIndexes[gDirIndexCount];
    int capacity = 64;
    x->names = (char **)malloc(sizeof(char *) * capacity);
    x->count = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL)
    {
        if (x->count == capacity)
        {
            capacity *= 2;
            x->names = (char **)realloc(x->names, sizeof(char *) * capacity);
        }
        x->names[x->count++] = strdup(e->d_name);
    }
    closedir(d);
    x->dir = strdup(dir);
    gDirIndexCount++;
    return x;
}

// The real spelling of path through the indexes: 1 found (in r), 0 not in its directory, -1 can't tell.
// r must have strlen(path) + 3 bytes.
static int index_lookup(const char *path, char *r)
{
    size_t len = strlen(path);
    char *p = (char *)alloca(len + 1);
    strcpy(p, path);

    // the directory the walk starts in: "/" or "sdmc:/" for absolute paths, "." for relative ones
    char *rest = p;
    char *colon = strchr(p, ':');
    if (p[0] == '/')
    {
        strcpy(r, "/");
        rest = p + 1;
    }
    else if (colon && colon[1] == '/' && strchr(p, '/') > colon)
    {
        size_t n = (size_t)(colon - p) + 2;
        memcpy(r, p, n);
        r[n] = 0;
        rest = colon + 2;
    }
    else
    {
        strcpy(r, ".");
    }

    char *c = strsep(&rest, "/\\");
    while (c)
    {
        char *next = strsep(&rest, "/\\");
        if (c[0] == 0 || strcmp(c, ".") == 0)
        {
            c = next;
            continue;
        }
        DirIndex *x = dir_index(r);
        if (!x)
            return -1;
        const char *found = NULL;
        for (int i = 0; i < x->count; i++)
        {
            if (strcasecmp(c, x->names[i]) == 0)
            {
                found = x->names[i];
                break;
            }
        }
        if (!found)
            return next ? -1 : 0;
        size_t rl = strlen(r);
        if (strcmp(r, ".") == 0)
            rl = 0;
        else if (r[rl - 1] != '/')
            r[rl++] = '/';
        strcpy(r + rl, found);
        c = next;
    }
    return 1;
}

static int is_write_mode(const char *mode)
{
    return strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+');
}

void fcaseopen_forget(void)
{
    pthread_mutex_lock(&gDirIndexLock);
    drop_dir_indexes();
    pthread_mutex_unlock(&gDirIndexLock);
}

void fcaseopen_stats(int *opens, int *misses, int *scans)
{
    pthread_mutex_lock(&gDirIndexLock);
    *opens = gStatOpens;
    *misses = gStatMisses;
    *scans = gStatScans;
    pthread_mutex_unlock(&gDirIndexLock);
}
#endif

FILE *fcaseopen(char const *path, char const *mode)
{
#if !defined(_WIN32)
    if (!is_write_mode(mode))
    {
        char *r = (char *)alloca(strlen(path) + 3);
        pthread_mutex_lock(&gDirIndexLock);
        gStatOpens++;
        int found = index_lookup(path, r);
        if (found == 0)
            gStatMisses++;
        pthread_mutex_unlock(&gDirIndexLock);
        if (found == 1)
            return fopen(r, mode);
        if (found == 0)
        {
            errno = ENOENT;
            return NULL;
        }
    }
    else
    {
        pthread_mutex_lock(&gDirIndexLock);
        drop_dir_indexes();
        pthread_mutex_unlock(&gDirIndexLock);
    }
#endif
#if defined(_WIN32)
    wchar_t *wpath = utf8_to_wide_alloc(path);
    wchar_t *wmode = utf8_to_wide_alloc(mode);
    if (!wpath || !wmode)
    {
        if (wpath) free(wpath);
        if (wmode) free(wmode);
        return NULL;
    }
    FILE *f = _wfopen(wpath, wmode);
    free(wpath);
    free(wmode);
#else
    FILE *f = fopen(path, mode);
#if !defined(_WIN32)
    if (!f)
    {
        char *r = (char *)alloca(strlen(path) + 3);
        if (casepath(path, r))
        {
            f = fopen(r, mode);
        }
    }
#endif
#endif
    return f;
}

int casechdir(char const *path)
{
#if !defined(_WIN32)
    char *r = (char *)alloca(strlen(path) + 3);
    if (casepath(path, r))
    {
        return chdir(r);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
#else
    wchar_t *wpath = utf8_to_wide_alloc(path);
    if (!wpath)
        return -1;
    int ret = _wchdir(wpath);
    free(wpath);
    return ret;
#endif
}

FILE *fcaseopenat(char const *base, char const *path, char const *mode)
{
#if !defined(_WIN32)
    if (!base || base[0] == '\0')
        base = ".";

    size_t baseLen = strlen(base);
    size_t pathLen = strlen(path);
    char *full = (char *)alloca(baseLen + pathLen + 3);

    strcpy(full, base);
    size_t fl = strlen(full);
    if (fl > 0 && full[fl - 1] != '/' && full[fl - 1] != '\\')
    {
        full[fl++] = '/';
        full[fl] = 0;
    }
    strcpy(full + fl, path);

    // switchfish: through fcaseopen and its directory indexes
    return fcaseopen(full, mode);
#else
    if (!base || base[0] == '\0')
        return fcaseopen(path, mode);

    size_t baseLen = strlen(base);
    size_t pathLen = strlen(path);
    char *full = (char *)alloca(baseLen + pathLen + 3);
    strcpy(full, base);
    size_t fl = strlen(full);
    if (fl > 0 && full[fl - 1] != '/' && full[fl - 1] != '\\')
    {
        full[fl++] = '/';
        full[fl] = 0;
    }
    strcpy(full + fl, path);
    return fcaseopen(full, mode);
#endif
}
