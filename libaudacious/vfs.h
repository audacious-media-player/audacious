#ifndef VFS_H
#define VFS_H

#include <glib.h>
#include <stdio.h>

typedef struct _VFSFile VFSFile;

G_BEGIN_DECLS

/* Reserved for private use by BMP */
gboolean vfs_init(void);

VFSFile * vfs_fopen(const gchar * path,
                    const gchar * mode);
gint vfs_fclose(VFSFile * file);

size_t vfs_fread(gpointer ptr,
                 size_t size,
                 size_t nmemb,
                 VFSFile * file);
size_t vfs_fwrite(gconstpointer ptr,
                  size_t size,
                  size_t nmemb,
                  VFSFile *file);

gint vfs_fseek(VFSFile * file,
               glong offset,
               gint whence);
void vfs_rewind(VFSFile * file);
glong vfs_ftell(VFSFile * file);

gboolean vfs_file_test(const gchar * path,
                       GFileTest test);
                    
gboolean vfs_is_writeable(const gchar * path);

gboolean vfs_truncate(VFSFile * file, glong length);


G_END_DECLS

#endif /* VFS_H */
