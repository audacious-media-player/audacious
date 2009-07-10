#include <glib.h>
#include <libaudcore/tuple.h>
#define TEST
#include <libaudtag/util.h>
#include <libaudtag/wma/wma.h>

#define WMA1_FILE       "test_wma1.wma"
#define WMA1_FILE_OUT   "test_wma1_out.wma"
#define WMA2_FILE  "test_wma2.wma"

int test_read(int n, char* f) {

    Tuple * test_tuple;
    test_tuple = tuple_new();

    tuple_associate_string(test_tuple, FIELD_FILE_PATH, NULL, f);

    test_tuple = wma_populate_tuple_from_file(test_tuple);

    const char *file_path = tuple_get_string(test_tuple, FIELD_FILE_PATH, NULL);

    if (g_ascii_strcasecmp(file_path, f)) {
        g_print("tagging failure on test %d:\n"
                "'%s' != '%s' \n",
                n, file_path, f);
        tuple_free(test_tuple);
        return EXIT_FAILURE;
    }
     return EXIT_SUCCESS;
}

int test_write(int n, char *f) {

    char *title = "Title 제목 ",
            *author = "Author المؤلف",
            *comment = "Comment Коментар",
            *album = "Album 相冊",
            *genre = "Genre शैली";
    Tuple * test_tuple = tuple_new();
    test_tuple = wma_populate_tuple_from_file(test_tuple);

    Tuple *write_test_tuple = makeTuple(test_tuple,
            title, author,
            comment, album,
            genre, "2111",
            f, 2);

    wma_write_tuple_to_file(write_test_tuple);
    wma_populate_tuple_from_file(test_tuple);

    const char* title_new = tuple_get_string(test_tuple, FIELD_TITLE, NULL);
    const char* author_new = tuple_get_string(test_tuple, FIELD_ARTIST, NULL);
    const char* comment_new = tuple_get_string(test_tuple, FIELD_COMMENT, NULL);
    const char* album_new = tuple_get_string(test_tuple, FIELD_ALBUM, NULL);
    const char* genre_new = tuple_get_string(test_tuple, FIELD_GENRE, NULL);

    if (g_ascii_strcasecmp(title, title_new) ||
            g_ascii_strcasecmp(author, author_new) ||
            g_ascii_strcasecmp(comment, comment_new) ||
            g_ascii_strcasecmp(album, album_new) ||
            g_ascii_strcasecmp(genre, genre_new)) {
        g_print("tagging failure on test %d:\n"
                "'%s' != '%s' \n"
                "'%s' != '%s' \n"
                "'%s' != '%s' \n"
                "'%s' != '%s' \n"
                "'%s' != '%s' \n",
                n,
                title, title_new,
                author, author_new,
                comment, comment_new,
                album, album_new,
                genre, genre_new);

        tuple_free(test_tuple);
        tuple_free(write_test_tuple);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int test_run(int argc, char *argv[]) {

    if (test_read(1, WMA1_FILE) == EXIT_FAILURE)
        return EXIT_FAILURE;

    if (test_read(2, WMA2_FILE) == EXIT_FAILURE)
        return EXIT_FAILURE;


    if (test_write(3, WMA1_FILE) == EXIT_FAILURE)
        return EXIT_FAILURE;

    if (test_write(4, WMA2_FILE) == EXIT_FAILURE)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
