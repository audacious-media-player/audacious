#include <taglib/tag.h>
#include <taglib/id3v2framefactory.h>

extern "C"
{
#include "tag_c_hacked.h"
}
using namespace TagLib;
using namespace ID3v2;
void taglib_set_id3v2_default_text_encoding()
{
  TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(TagLib::String::UTF8);
}

