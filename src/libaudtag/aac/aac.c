
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>
#include "../tag_module.h"
#include "aac.h"
#include "../util.h"



tag_module_t aac = {aac_can_handle_file, aac_populate_tuple_from_file, aac_write_tuple_to_file};

gchar* atom_types[] = {"\251alb", "\251nam", "cprt", "\251art", "\251ART", "trkn", "\251day", "gnre", "desc"};

Atom *readAtom(VFSFile *fd,Atom *atom)
{    
    atom->size = read_BEint32(fd);
    atom->name = read_ASCII(fd,4);  
    atom->body = read_ASCII(fd,atom->size-8);

    return atom;
}

void writeAtom(VFSFile *fd, Atom *atom)
{     
    write_BEint32(fd,atom->size);
    vfs_fwrite(atom->name,4,1,fd);
    vfs_fwrite(atom->body,atom->size-8,1,fd);
}

void printAtom(Atom *atom)
{
     DEBUG("size = %x\n",atom->size); 
     DEBUG("name = %s\n",atom->name);
}

StrDataAtom *readStrDataAtom(VFSFile *fd,StrDataAtom *atom)
{
    atom->atomsize = read_BEint32(fd);
    atom->name = read_ASCII(fd,4);
    atom->datasize = read_BEint32(fd);
    atom->dataname = read_ASCII(fd,4);
    atom->vflag = read_BEint32(fd);
    atom->nullData = read_BEint32(fd);
    atom->data = read_ASCII(fd,atom->datasize-16);

    return atom;
}

void writeStrDataAtom(VFSFile *fd,StrDataAtom *atom)
{
    write_BEint32(fd,atom->atomsize);
    vfs_fwrite(atom->name,4,1,fd);
    write_BEint32(fd,atom->datasize);
    vfs_fwrite(atom->dataname,4,1,fd);
    write_BEint32(fd,atom->vflag);
    write_BEint32(fd,atom->nullData);
    vfs_fwrite(atom->data,atom->datasize-16,1,fd);
}

Atom *findAtom(VFSFile *fd,gchar* name)
{
    Atom *atom  = g_new0(Atom,1);
    atom = readAtom(fd,atom);
    while(g_strcmp0(atom->name,name) && !vfs_feof(fd))
    {
        g_free(atom);
        atom = g_new0(Atom,1);
        atom = readAtom(fd,atom);
    }
    if(vfs_feof(fd))
        return NULL;
    return atom;
}


Atom *getilstAtom(VFSFile *fd)
{

    Atom *moov = findAtom(fd,MOOV);
    //jump back the body of this atom and search his childs
    vfs_fseek(fd,-(moov->size-7), SEEK_CUR);
    //find the udta atom
    Atom *udta = findAtom(fd,UDTA);
    vfs_fseek(fd,-(udta->size-7), SEEK_CUR);
    Atom *meta = findAtom(fd,META);
    vfs_fseek(fd,-(meta->size-11), SEEK_CUR);
    Atom *ilst = findAtom(fd,ILST);
    vfs_fseek(fd,-(ilst->size-7),SEEK_CUR);

    return ilst;

}


gboolean aac_can_handle_file(VFSFile *f)
{
/*
    Atom *ilst  = g_new0(Atom,1);
    ilst = getilstAtom(f);
    printAtom(ilst);
*/
    return TRUE;
}

int getAtomID(gchar* name) {
    int i = 0;
    for (i = 0; i < MP4_ITEMS_NO; i++) {
        if (!g_strcmp0(name, atom_types[i]))
            return i;
    }
    return -1;
}

Tuple *aac_populate_tuple_from_file(Tuple *tuple,VFSFile *f)
{
    Atom *atom = getilstAtom(f);
    int size_read = 0;

    if(dataAtoms != NULL)
    {
        mowgli_node_t *n, *tn;
        MOWGLI_LIST_FOREACH_SAFE(n, tn, dataAtoms->head)
        {
            mowgli_node_delete(n,dataAtoms);
        }
    }
    dataAtoms = mowgli_list_create();

    while (size_read < atom->size)
    {
        StrDataAtom *a = g_new0(StrDataAtom,1);
        Atom *at = g_new0(Atom,1);
        at = readAtom(f,at);        
        int atomtype = getAtomID(at->name);
        if(atomtype == -1)
        {
            size_read += at->size;
            mowgli_node_add(at, mowgli_node_create(), dataAtoms);
            continue;
        }
        vfs_fseek(f,-(at->size),SEEK_CUR);
        a = readStrDataAtom(f,a);
        size_read += a->atomsize;
/*
        mowgli_node_add(a, mowgli_node_create(), dataAtoms);
*/

        switch (atomtype)
        {
            case MP4_ALBUM:
            {
                tuple_associate_string(tuple,FIELD_ALBUM,NULL,a->data);
            }break;
            case MP4_TITLE:
            {
                tuple_associate_string(tuple,FIELD_TITLE,NULL,a->data);
            }break;
            case MP4_COPYRIGHT:
            {
                tuple_associate_string(tuple,FIELD_COPYRIGHT,NULL,a->data);
            }break;
            case MP4_ARTIST:
            case MP4_ARTIST2:
            {
                tuple_associate_string(tuple,FIELD_ARTIST,NULL,a->data);
            }break;
            case MP4_TRACKNR:
            {
                //tuple_associate_string(tuple,FIELD_ALBUM,NULL,a->data);
            }break;
            case MP4_YEAR:
            {
                tuple_associate_int(tuple,FIELD_YEAR,NULL,atoi(a->data));
            }break;
            case MP4_GENRE:
            {
                guint8 *val = (guint8*)(a->data + (a->datasize-17));
                const gchar* genre = ID3v1GenreList[*val-1];
                tuple_associate_string(tuple,FIELD_GENRE,NULL,genre);
            }break;
            case MP4_COMMENT:
            {
                tuple_associate_string(tuple,FIELD_COMMENT,NULL,a->data);
            }break;
        }
    }
    return tuple;
}

gboolean aac_write_tuple_to_file(Tuple* tuple, VFSFile *f)
{
    return FALSE;
}