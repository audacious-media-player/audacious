#ifndef TAG_WMA_MODULE_H

#define TAG_WMA_MODULE_H

gboolean can_handle(Tuple *tuple);

Tuple *populate_tuple_from_file(Tuple* tuple);

gboolean write_tuple_to_file(Tuple* tuple);

extern tag_module_t wma;
#endif
