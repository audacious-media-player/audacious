#ifndef VFS_H
#define VFS_H

#include <glib.h>
#include <stdio.h>

typedef struct _VFSFile VFSFile;

G_BEGIN_DECLS

/* Reserved for private use by BMP */
extern gboolean vfs_init(void);

extern VFSFile * vfs_fopen(const gchar * path,
                    const gchar * mode);
extern gint vfs_fclose(VFSFile * file);

extern size_t vfs_fread(gpointer ptr,
                 size_t size,
                 size_t nmemb,
                 VFSFile * file);
extern size_t vfs_fwrite(gconstpointer ptr,
                  size_t size,
                  size_t nmemb,
                  VFSFile *file);

extern gint vfs_fseek(VFSFile * file,
               glong offset,
               gint whence);
extern void vfs_rewind(VFSFile * file);
extern glong vfs_ftell(VFSFile * file);
extern gboolean vfs_feof(VFSFile * file);

extern gboolean vfs_file_test(const gchar * path,
                       GFileTest test);

extern gboolean vfs_is_writeable(const gchar * path);

extern gboolean vfs_truncate(VFSFile * file, glong length);


G_END_DECLS

#endif /* VFS_H */
