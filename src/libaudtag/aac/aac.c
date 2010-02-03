
#include <glib/gstdio.h>
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>
#include "../tag_module.h"
#include "aac.h"
#include "../util.h"

static const char* ID3v1GenreList[] = {
                                       "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
                                       "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies",
                                       "Other", "Pop", "R&B", "Rap", "Reggae", "Rock",
                                       "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
                                       "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
                                       "Fusion", "Trance", "Classical", "Instrumental", "Acid", "House",
                                       "Game", "Sound Clip", "Gospel", "Noise", "AlternRock", "Bass",
                                       "Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
                                       "Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk",
                                       "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
                                       "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret",
                                       "New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi",
                                       "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical",
                                       "Rock & Roll", "Hard Rock", "Folk", "Folk/Rock", "National Folk", "Swing",
                                       "Fast-Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde",
                                       "Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",
                                       "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson",
                                       "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus",
                                       "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
                                       "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
                                       "Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall",
                                       "Goa", "Drum & Bass", "Club House", "Hardcore", "Terror",
                                       "Indie", "BritPop", "NegerPunk", "Polsk Punk", "Beat",
                                       "Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover", "Contemporary C",
                                       "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop",
                                       "SynthPop",
};

gchar* atom_types[] = {"\251alb", "\251nam", "cprt", "\251art", "\251ART", "trkn", "\251day", "gnre", "desc"};

tag_module_t aac = {aac_can_handle_file, aac_populate_tuple_from_file, aac_write_tuple_to_file};

Atom *readAtom(VFSFile *fd) {
    Atom *atom = g_new0(Atom, 1);
    atom->size = read_BEuint32(fd);
    atom->name = read_char_data(fd, 4);
    atom->body = read_char_data(fd, atom->size - 8);
    atom->type = 0;
    return atom;
}

void writeAtom(VFSFile *fd, Atom *atom) {
    write_BEuint32(fd, atom->size);
    vfs_fwrite(atom->name, 4, 1, fd);
    vfs_fwrite(atom->body, atom->size - 8, 1, fd);
}

void printAtom(Atom *atom) {
    AUDDBG("size = %x\n", atom->size);
    AUDDBG("name = %s\n", atom->name);
}

StrDataAtom *readStrDataAtom(VFSFile *fd) {
    StrDataAtom *atom = g_new0(StrDataAtom, 1);
    atom->atomsize = read_BEuint32(fd);
    atom->name = read_char_data(fd, 4);
    atom->datasize = read_BEuint32(fd);
    atom->dataname = read_char_data(fd, 4);
    atom->vflag = read_BEuint32(fd);
    atom->nullData = read_BEuint32(fd);
    atom->data = read_char_data(fd, atom->datasize - 16);
    atom->type = 1;
    return atom;
}

void writeStrDataAtom(VFSFile *fd, StrDataAtom *atom) {
    write_BEuint32(fd, atom->atomsize);
    vfs_fwrite(atom->name, 4, 1, fd);
    write_BEuint32(fd, atom->datasize);
    vfs_fwrite(atom->dataname, 4, 1, fd);
    write_BEuint32(fd, atom->vflag);
    write_BEuint32(fd, atom->nullData);
    vfs_fwrite(atom->data, atom->datasize - 16, 1, fd);
}

Atom *findAtom(VFSFile *fd, gchar* name) {
    Atom *atom = readAtom(fd);
    while (strcmp(atom->name, name) && !vfs_feof(fd))
    {
        g_free(atom);
        atom = readAtom(fd);
    }
    if (vfs_feof(fd))
    {
        g_free(atom);
        AUDDBG("The atom %s could not be found\n", name);
        return NULL;
    }
    return atom;
}

Atom *getilstAtom(VFSFile *fd) {
    Atom *moov = findAtom(fd, MOOV);

    // search atom childs
    vfs_fseek(fd, -(moov->size - 7), SEEK_CUR);
    Atom *udta = findAtom(fd, UDTA);


    vfs_fseek(fd, -(udta->size - 7), SEEK_CUR);
    Atom *meta = findAtom(fd, META);

    vfs_fseek(fd, -(meta->size - 11), SEEK_CUR);
    Atom *ilst = findAtom(fd, ILST);

    int zz = vfs_ftell(fd);
    AUDDBG("zzz = %d\n", zz);
    ilstFileOffset = vfs_ftell(fd) - ilst->size;
    vfs_fseek(fd, -(ilst->size - 7), SEEK_CUR);

    return ilst;

}

int getAtomID(gchar* name) {
    g_return_val_if_fail(name != NULL, -1);
    int i = 0;
    for (i = 0; i < MP4_ITEMS_NO; i++)
    {
        if (!strcmp(name, atom_types[i]))
            return i;
    }
    return -1;
}

StrDataAtom *makeAtomWithData(const gchar* data, StrDataAtom *atom, int field) {
    guint32 charsize = strlen(data);
    atom->atomsize = charsize + 24;
    atom->name = atom_types[field];
    atom->datasize = charsize + 16;
    atom->dataname = "data";
    atom->vflag = 0x0;
    atom->nullData = 0x0;
    atom->data = (gchar*) data;
    atom->type = 1;
    return atom;

}

void writeAtomListToFile(VFSFile* from, VFSFile *to, int offset, mowgli_list_t *list) {
    //read free atom if we have any :D
    guint32 oset = ilstFileOffset + ilstAtom->size;
    vfs_fseek(from, oset, SEEK_SET);
    mowgli_list_t *atoms_before_free = mowgli_list_create();
    Atom *atom = readAtom(from);
    while (strcmp(atom->name, "free") && !vfs_feof(from))
    {
        mowgli_node_add(atom, mowgli_node_create(), atoms_before_free);
        g_free(atom);
        atom = readAtom(from);
    }
    g_free(atom);
    if (vfs_feof(from))
    {
        AUDDBG("No free atoms\n");
        g_free(atom);
        atom = NULL;
    }

    //write ilst atom header
    gchar il[4] = ILST;
    vfs_fwrite(&newilstSize, 4, 1, to);
    vfs_fwrite(il, 4, 1, to);
    //write ilst

    mowgli_node_t *n, *tn;

    MOWGLI_LIST_FOREACH_SAFE(n, tn, list->head) {
        if (((Atom*) (n->data))->type == 0)
        {
            writeAtom(to, (Atom*) (n->data));
        } else
        {
            writeStrDataAtom(to, (StrDataAtom*) (n->data));
        }
    }

    //write all atoms before free
    if (atoms_before_free->count != 0)
    {

        MOWGLI_LIST_FOREACH_SAFE(n, tn, list->head) {
            writeAtom(to, (Atom*) (n->data));
        }
    }
    if (atom != NULL)
    {
        atom->size -= newilstSize - ilstAtom->size;
    }
    writeAtom(to, atom);
}

gboolean aac_can_handle_file(VFSFile *f) {
    Atom *first_atom = readAtom(f);
    if (!strcmp(first_atom->name, FTYP))
        return TRUE;
    return FALSE;
}

Tuple *aac_populate_tuple_from_file(Tuple *tuple, VFSFile *f) {
    if (ilstAtom)
        g_free(ilstAtom);
    ilstAtom = getilstAtom(f);
    int size_read = 0;

    if (dataAtoms != NULL)
    {
        mowgli_node_t *n, *tn;

        MOWGLI_LIST_FOREACH_SAFE(n, tn, dataAtoms->head) {
            mowgli_node_delete(n, dataAtoms);
        }
    }
    dataAtoms = mowgli_list_create();

    while (size_read < ilstAtom->size)
    {
        Atom *at = readAtom(f);
        mowgli_node_add(at, mowgli_node_create(), dataAtoms);
        int atomtype = getAtomID(at->name);
        if (atomtype == -1)
        {
            size_read += at->size;
            continue;
        }
        g_free(at);
        vfs_fseek(f, -(at->size), SEEK_CUR);
        StrDataAtom *a = readStrDataAtom(f);
        size_read += a->atomsize;

        switch (atomtype)
        {
            case MP4_ALBUM:
            {
                tuple_associate_string(tuple, FIELD_ALBUM, NULL, a->data);
            }
                break;
            case MP4_TITLE:
            {
                tuple_associate_string(tuple, FIELD_TITLE, NULL, a->data);
            }
                break;
            case MP4_COPYRIGHT:
            {
                tuple_associate_string(tuple, FIELD_COPYRIGHT, NULL, a->data);
            }
                break;
            case MP4_ARTIST:
            case MP4_ARTIST2:
            {
                tuple_associate_string(tuple, FIELD_ARTIST, NULL, a->data);
            }
                break;
            case MP4_TRACKNR:
            {
                //tuple_associate_string(tuple,FIELD_ALBUM,NULL,a->data);
            }
                break;
            case MP4_YEAR:
            {
                tuple_associate_int(tuple, FIELD_YEAR, NULL, atoi(a->data));
            }
                break;
            case MP4_GENRE:
            {
                guint8 *val = (guint8*) (a->data + (a->datasize - 17));
                const gchar* genre = ID3v1GenreList[*val - 1];
                tuple_associate_string(tuple, FIELD_GENRE, NULL, genre);
            }
                break;
            case MP4_COMMENT:
            {
                tuple_associate_string(tuple, FIELD_COMMENT, NULL, a->data);
            }
                break;
        }
    }
    return tuple;
}

gboolean aac_write_tuple_to_file(Tuple* tuple, VFSFile *f) {
#ifdef BROKEN
    return FALSE;
#endif
    newilstSize = 0;
    mowgli_node_t *n, *tn;
    mowgli_list_t *newdataAtoms;
    newdataAtoms = mowgli_list_create();

    MOWGLI_LIST_FOREACH_SAFE(n, tn, dataAtoms->head) {
        int atomtype = getAtomID(((StrDataAtom*) (n->data))->name);
        switch (atomtype)
        {
            case MP4_ALBUM:
            {
                const gchar* strVal = tuple_get_string(tuple, FIELD_ALBUM, NULL);
                if (strVal != NULL)
                {
                    StrDataAtom *atom = g_new0(StrDataAtom, 1);
                    atom = makeAtomWithData(strVal, atom, MP4_ALBUM);
                    mowgli_node_add(atom, mowgli_node_create(), newdataAtoms);
                    newilstSize += atom->atomsize;
                } else
                {
                    mowgli_node_add(n->data, mowgli_node_create(), newdataAtoms);
                    newilstSize += ((Atom*) (n->data))->size;
                }
            }
                break;
            case MP4_TITLE:
            {
                const gchar* strVal = tuple_get_string(tuple, FIELD_TITLE, NULL);
                if (strVal != NULL)
                {
                    StrDataAtom *atom = g_new0(StrDataAtom, 1);
                    atom = makeAtomWithData(strVal, atom, MP4_TITLE);
                    mowgli_node_add(atom, mowgli_node_create(), newdataAtoms);
                    newilstSize += atom->atomsize;
                } else
                {
                    mowgli_node_add(n->data, mowgli_node_create(), newdataAtoms);
                    newilstSize += ((Atom*) (n->data))->size;
                }
            }
                break;
            case MP4_COPYRIGHT:
            {
                const gchar* strVal = tuple_get_string(tuple, FIELD_COPYRIGHT, NULL);
                if (strVal != NULL)
                {
                    StrDataAtom *atom = g_new0(StrDataAtom, 1);
                    atom = makeAtomWithData(strVal, atom, MP4_COPYRIGHT);
                    mowgli_node_add(atom, mowgli_node_create(), newdataAtoms);
                    newilstSize += atom->atomsize;
                } else
                {
                    mowgli_node_add(n->data, mowgli_node_create(), newdataAtoms);
                    newilstSize += ((Atom*) (n->data))->size;
                }
            }
                break;
            case MP4_ARTIST:
            case MP4_ARTIST2:
            {
                const gchar* strVal = tuple_get_string(tuple, FIELD_ARTIST, NULL);
                if (strVal != NULL)
                {
                    StrDataAtom *atom = g_new0(StrDataAtom, 1);
                    atom = makeAtomWithData(strVal, atom, MP4_ARTIST2);
                    mowgli_node_add(atom, mowgli_node_create(), newdataAtoms);
                    newilstSize += atom->atomsize;
                } else
                {
                    mowgli_node_add(n->data, mowgli_node_create(), newdataAtoms);
                    newilstSize += ((Atom*) (n->data))->size;
                }
            }
                break;
            case MP4_TRACKNR:
            {
                //tuple_associate_string(tuple,FIELD_ALBUM,NULL,a->data);
            }
                break;
            case MP4_YEAR:
            {
                int iyear = tuple_get_int(tuple, FIELD_YEAR, NULL);
                gchar* strVal = g_strdup_printf("%d", iyear);
                if (strVal != NULL)
                {
                    StrDataAtom *atom = g_new0(StrDataAtom, 1);
                    atom = makeAtomWithData(strVal, atom, MP4_YEAR);
                    mowgli_node_add(atom, mowgli_node_create(), newdataAtoms);
                    newilstSize += atom->atomsize;
                } else
                {
                    mowgli_node_add(n->data, mowgli_node_create(), newdataAtoms);
                    newilstSize += ((Atom*) (n->data))->size;
                }
            }
                break;
                /*
                            case MP4_GENRE:
                            {

                                guint8 *val = (guint8*)(a->data + (a->datasize-17));
                                const gchar* genre = ID3v1GenreList[*val-1];
                                tuple_associate_string(tuple,FIELD_GENRE,NULL,genre);

                            }break;
                 */
            case MP4_COMMENT:
            {
                const gchar* strVal = tuple_get_string(tuple, FIELD_COMMENT, NULL);
                if (strVal != NULL)
                {
                    StrDataAtom *atom = g_new0(StrDataAtom, 1);
                    atom = makeAtomWithData(strVal, atom, MP4_COMMENT);
                    mowgli_node_add(atom, mowgli_node_create(), newdataAtoms);
                    newilstSize += atom->atomsize;
                } else
                {
                    mowgli_node_add(n->data, mowgli_node_create(), newdataAtoms);
                    newilstSize += ((Atom*) (n->data))->size;
                }
            }
                break;
            default:
            {
                mowgli_node_add(n->data, mowgli_node_create(), newdataAtoms);
                newilstSize += ((Atom*) (n->data))->size;
            }
                break;
        }
    }

    VFSFile *tmp;
    const gchar *tmpdir = g_get_tmp_dir();
    gchar *tmp_path = g_strdup_printf("file://%s/%s", tmpdir, "tmp.mp4");
    tmp = vfs_fopen(tmp_path, "w");
    copyAudioData(f, tmp, 0, ilstFileOffset);
    writeAtomListToFile(f, tmp, ilstFileOffset, newdataAtoms);
    return TRUE;
}
