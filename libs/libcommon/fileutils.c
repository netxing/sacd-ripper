/**
 * SACD Ripper - https://github.com/sacd-ripper/
 *
 * Copyright (c) 2010-2015 by respective authors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <wchar.h>

#include "logging.h"
#include "fileutils.h"
#include "charset.h"
#include "utils.h"


int stat_wrap(const char *pathname, struct stat *buf)
{
    int ret;

#if defined(__MINGW32__) || defined(WIN32) || defined(_WIN32)
    // Note buf is not in _stat type so buf is untouched.
    wchar_t *w_pathname;
    struct _stat buffer;

    w_pathname = (wchar_t *)charset_convert(pathname, strlen(pathname), "UTF-8", "UCS-2-INTERNAL");
    ret = _wstat(w_pathname, &buffer);
    free(w_pathname);
#else
    ret = stat(pathname, buf);
#endif
    return ret;
}

//   Test if exist dir
//    path to input dir
//    Return 0 = path not exists
//    Return 1 = path exist
int path_dir_exists(char * path)
{
    int path_exist=0;
    int ret=0;
#if defined(WIN32) || defined(_WIN32) || defined(_MSC_VER)
    struct _stat fileinfo_win;
#else
    struct stat fileinfo;
#endif

#if defined(WIN32) || defined(_WIN32)
    wchar_t *w_pathname;
    w_pathname = (wchar_t *)charset_convert(path, strlen(path), "UTF-8", "UCS-2-INTERNAL");
    ret = _wstat(w_pathname, &fileinfo_win);
    free(w_pathname);
    if (ret == 0 && (fileinfo_win.st_mode & _S_IFMT) == _S_IFDIR)
     path_exist=1;
#else
    ret = stat(path, &fileinfo);
    if (ret == 0 && S_ISDIR(fileinfo.st_mode))
     path_exist=1;
#endif
 return path_exist;
}


// construct a filename from various parts
//
// path - the path the file is placed in (don't include a trailing '/')
// dir - the parent directory of the file (don't include a trailing '/')
// file - the filename
// extension - the suffix of a file (don't include a leading '.')
//             assume the length of extenstion must be 3 chars
// NOTE: caller must free the returned string!
// NOTE: any of the parameters may be NULL to be omitted
char *make_filename(const char *path, const char *dir, const char *filename, const char *extension)
{
    char * ret = NULL;
    int  pos   = 0;
    char string_buf[1024];  // keep the full path

    memset(string_buf,0,sizeof(string_buf));

    if (path)
    {
        strncpy(string_buf, path, min(strlen(path), sizeof(string_buf) - 5)); // (-4 => making room for dot + extension!!!)
        pos += min(strlen(path), sizeof(string_buf) - 5);
#if defined(WIN32) || defined(_WIN32)
        string_buf[pos] = '\\';
#else
        string_buf[pos] = '/';
#endif
        pos++;
    }
    if (dir)
    {       
        strncpy(string_buf+pos,dir,min(strlen(dir),sizeof(string_buf) -pos-5));
        pos += min(strlen(dir), sizeof(string_buf) - pos - 5);
#if defined(WIN32) || defined(_WIN32)
        string_buf[pos] = '\\';
#else
        string_buf[pos] = '/';
#endif
        pos++;      
    }

    sanitize_filepath(string_buf);

    if (filename)
    {
        char filename_duplicate[512];
        memset(filename_duplicate, 0, sizeof(filename_duplicate));
        strncpy(filename_duplicate, filename, min(strlen(filename), sizeof(filename_duplicate) - 1));
        sanitize_filename(filename_duplicate);

        strncpy(string_buf + pos, filename_duplicate, min(strlen(filename_duplicate), sizeof(string_buf) - pos - 5)); 
        pos += min(strlen(filename_duplicate), sizeof(string_buf) - pos - 5);
        }
    if (extension)
    {
        string_buf[pos] = '.';
        pos++;
        strncpy(string_buf + pos, extension, min(strlen(extension), sizeof(string_buf) - pos - 1));
        pos += min(strlen(extension), sizeof(string_buf) - pos - 1);
    }

    ret = strdup(string_buf);
    if (ret == NULL)
    {
        // LOG(lm_main, LOG_ERROR, ("calloc(sizeof(char) * len) failed. Out of memory."));
        printf("Error! make_filename: Cannot alocate memory Out of memory.\n");
        return NULL;
    }
    return ret;
}

// substitute various items into a formatted string (similar to printf)
//
// format - the format of the filename
// tracknum - gets substituted for %N in format
// year - gets substituted for %Y in format
// artist - gets substituted for %A in format
// album - gets substituted for %L in format
// title - gets substituted for %T in format
//
// NOTE: caller must free the returned string!
char * parse_format(const char * format, int tracknum, const char * year, const char * artist, const char * album, const char * title)
{
    unsigned i     = 0;
    int      len   = 0;
    char     * ret = NULL;
    int      pos   = 0;

    for (i = 0; i < strlen(format); i++)
    {
        if ((format[i] == '%') && (i + 1 < strlen(format)))
        {
            switch (format[i + 1])
            {
            case 'A':
                if (artist)
                    len += strlen(artist);
                break;
            case 'L':
                if (album)
                    len += strlen(album);
                break;
            case 'N':
                if ((tracknum > 0) && (tracknum < 100))
                    len += 2;
                break;
            case 'Y':
                if (year)
                    len += strlen(year);
                break;
            case 'T':
                if (title)
                    len += strlen(title);
                break;
            case '%':
                len += 1;
                break;
            }

            i++;             // skip the character after the %
        }
        else
        {
            len++;
        }
    }

    ret = malloc(sizeof(char) * (len + 1));
    if (ret == NULL)
        LOG(lm_main, LOG_ERROR, ("malloc(sizeof(char) * (len+1)) failed. Out of memory."));

    for (i = 0; i < strlen(format); i++)
    {
        if ((format[i] == '%') && (i + 1 < strlen(format)))
        {
            switch (format[i + 1])
            {
            case 'A':
                if (artist)
                {
                    strncpy(&ret[pos], artist, strlen(artist));
                    pos += strlen(artist);
                }
                break;
            case 'L':
                if (album)
                {
                    strncpy(&ret[pos], album, strlen(album));
                    pos += strlen(album);
                }
                break;
            case 'N':
                if ((tracknum > 0) && (tracknum < 100))
                {
                    ret[pos]     = '0' + ((int) tracknum / 10);
                    ret[pos + 1] = '0' + (tracknum % 10);
                    pos         += 2;
                }
                break;
            case 'Y':
                if (year)
                {
                    strncpy(&ret[pos], year, strlen(year));
                    pos += strlen(year);
                }
                break;
            case 'T':
                if (title)
                {
                    strncpy(&ret[pos], title, strlen(title));
                    pos += strlen(title);
                }
                break;
            case '%':
                ret[pos] = '%';
                pos     += 1;
            }

            i++;             // skip the character after the %
        }
        else
        {
            ret[pos] = format[i];
            pos++;
        }
    }
    ret[pos] = '\0';

    return ret;
}

#ifdef __lv2ppu__
#include <sys/stat.h>
#include <sys/file.h>
#endif


// Uses mkdir() for every component of the path except base_dir
//  input: path_and_name (including base_dir)
//         base_dir (from where to start creating directories)
//         base_dir can be NULL
// returns if any of those fails with anything other than EEXIST.
//
int recursive_mkdir(char* path_and_name,char * base_dir, mode_t mode)
{
    int  count;
    int  path_and_name_length = 0;
    int  rc;
    char charReplaced;
    char * pos=NULL;

    if(path_and_name==NULL)return -1;

    if(base_dir == NULL)
        pos = path_and_name;
    else
        pos = path_and_name + strlen(base_dir)+1;

    path_and_name_length = strlen(pos);

    for (count = 0; count < path_and_name_length; count++)
    {
        if (pos[count] == '/' || pos[count] == '\\')
        {
            if(count==0)continue;  // skip first trailing slash

            charReplaced             = pos[count];
            pos[count] = '\0';

#if defined(WIN32) || defined(_WIN32)
            {
                wchar_t *wide_path_and_name = (wchar_t *) charset_convert(path_and_name, strlen(path_and_name), "UTF-8", "UCS-2-INTERNAL");
                rc = _wmkdir(wide_path_and_name);
                free(wide_path_and_name);
            }
#else
            
            rc = mkdir(path_and_name, mode);
#endif            

#ifdef __lv2ppu__
            sysFsChmod(path_and_name, S_IFMT | 0777); 
#elif !defined(_WIN32)
            chmod(path_and_name, mode);
#endif
            pos[count] = charReplaced;

            if (rc != 0 && !(errno == EEXIST || errno == EISDIR || errno == EACCES || errno == EROFS))
                return rc;
        }
    } // end for count


    // in case the path doesn't have a trailing slash:
#if defined(WIN32) || defined(_WIN32)
    {
        wchar_t *wide_path_and_name = (wchar_t *) charset_convert(path_and_name, strlen(path_and_name), "UTF-8", "UCS-2-INTERNAL");
        rc = _wmkdir(wide_path_and_name);
        free(wide_path_and_name);
    }
#else
    rc = mkdir(path_and_name, mode);
#endif

#ifdef __lv2ppu__
    sysFsChmod(path_and_name, S_IFMT | 0777);
#elif !defined(_WIN32)
    chmod(path_and_name, mode);
#endif
    if (rc != 0 && !(errno == EEXIST || errno == EISDIR || errno == EACCES || errno == EROFS))
        return rc;
    else
        return 0;
}

// Uses mkdir() for every component of the path except the last one,
// and returns if any of those fails with anything other than EEXIST.
int recursive_parent_mkdir(char* path_and_name, mode_t mode)
{
    int count;
    int have_component = 0;
    int rc             = 1; // guaranteed fail unless mkdir is called


    // find the last component and cut it off
    for (count = strlen(path_and_name) - 1; count >= 0; count--)
    {
        if (path_and_name[count] != '/' && path_and_name[count] != '\\')
            have_component = 1;

        if ((path_and_name[count] == '/' && have_component) ||
            (path_and_name[count] == '\\' && have_component) )
        {
            path_and_name[count] = 0;

#if defined(WIN32) || defined(_WIN32)
            {
                wchar_t *wide_path_and_name = (wchar_t *) charset_convert(path_and_name, strlen(path_and_name), "UTF-8", "UCS-2-INTERNAL");
                rc = _wmkdir(wide_path_and_name);
                free(wide_path_and_name);
            }
#else
            rc = mkdir(path_and_name, 0777); // 0774 // S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH
#endif
            path_and_name[count] = '/';
        }
    }

    return rc;
}

char *get_unique_path(char *dir, char *file, const char *ext)
{
    char *path;
    char *file_new;
    unsigned int i = 0;
    struct stat stat_file;

    file_new = (char *)malloc((strlen(file)+16)*sizeof(char));
    strcpy(file_new, file);
    for(i = 0; i < 64; i ++){
        if(i){
            snprintf(file_new, strlen(file)+8, "%s (%d)", file, i);
        }
        path = make_filename(dir, 0, file_new, ext);
        if (stat_wrap(path, &stat_file) != 0)       
        {
                free(path);
                path = make_filename(dir, 0, file_new, ext);
                break;
        }
        free(path);
        path = 0;
    }
    free(file_new);
    if(!path){
        return 0;
    }
    return path;
}
// Construct an unique filename
//  *device
//  *dir
//  *file -  path file
//  *extension
//  NOTE: caller must free the returned string!
//
char * get_unique_filename(char *dev,char *dir, char *file, char *ext)
{
    struct stat stat_file;
    int file_exists=0, count = 1;

    int len = strlen(file) + 10;

    char *total_path = make_filename(dev, dir, file, ext);

    file_exists = (stat_wrap(total_path, &stat_file) == 0) ? 1 :0;

    while (file_exists==1)
    {
        free(total_path);
        char *file_copy = (char *) calloc(len, sizeof(char));
        snprintf(file_copy, len, "%s (%d)", file, count++);
        total_path = make_filename(dev, dir, file_copy, ext);
        file_exists = (stat_wrap(total_path, &stat_file) == 0)? 1:0;

        if (count > 20) break; // loop must be stoped somewhere
    }
    return total_path;
}

// Construct an unique directory
//
//  *device -  device path
//  **dir - directory
//  NOTE: in case device is not NULL it removes trailing slash in returned string
//  NOTE: caller must free the returned string!
//
char * get_unique_dir(char *device, char *dir)
{ 

    int dir_exists = 0, count = 1;
    int len = strlen(dir) + 10;

    char *device_dir = make_filename(device, dir , NULL,NULL);

    dir_exists  = path_dir_exists(device_dir);
    while (dir_exists == 1)
    {
        free(device_dir);
        
        char *dir_copy = calloc(len, sizeof(char));
        snprintf(dir_copy, len, "%s (%d)", dir, count++);

        device_dir = make_filename(device, dir_copy, NULL, NULL);
        dir_exists = path_dir_exists(device_dir);
        if (count > 20)
            break; // loop must be stoped somewhere
    }
       
        
        // Remove the trailing slash
#if defined(WIN32) || defined(_WIN32)
        if (device_dir[strlen(device_dir) - 1] == '\\')
        {
#else
        if (device_dir[strlen(device_dir) - 1] == '/')
        {
#endif
            device_dir[strlen(device_dir) - 1] = '\0';
        }

    return device_dir;
}

void sanitize_filename(char *f)
{
    const char unsafe_chars[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
                                 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
                                 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x22, 0x2a, 0x2f, 0x3a, 0x3c, 0x3e, 0x3f, 0x5c,
                                 0x7c, 0x7f, 0x00};

    char *c = f;

    if (!c || strlen(c) == 0)
        return;

    for (; *c; c++)
    {
        if (strchr(unsafe_chars, *c))
            *c = ' ';
    }
    replace_double_space_with_single(f);
    trim_whitespace(f);
}

static void trim_dots(char * s) 
{
    char * p = s;
    int l = strlen(p);

    while(p[l - 1] == '.') p[--l] = 0;
    while(*p && *p == '.') ++p, --l;

    memmove(s, p, l + 1);
}

void sanitize_filepath(char *f)
{
    const char unsafe_chars[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
                                 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
                                 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x22, 0x2a, 0x3c, 0x3e, 0x3f, 0x7c, 0x7f, 0x00};

    char *c = f;

    if (!c || strlen(c) == 0)
        return;

    for (; *c; c++)
    {
        if (strchr(unsafe_chars, *c))
            *c = ' ';
    }
    replace_double_space_with_single(f);

    trim_whitespace(f);
    trim_dots(f);
    trim_whitespace(f);
}
