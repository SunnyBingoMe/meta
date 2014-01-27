/**
 * @file corpus.cpp
 * @author Sean Massung
 */

#include "corpus/corpus.h"
#include "corpus/all.h"
#include "cpptoml.h"
#include "util/shim.h"

namespace meta {
namespace corpus {

std::unique_ptr<corpus> corpus::load(const std::string & config_file)
{
    auto config = cpptoml::parse_file(config_file);

    auto type = config.get_as<std::string>("corpus-type");
    if (!type)
        throw corpus_exception{"corpus-type missing from configuration file"};

    auto prefix = config.get_as<std::string>("prefix");
    if (!prefix)
        throw corpus_exception{"prefix missing from configuration file"};

    auto dataset = config.get_as<std::string>("dataset");
    if (!dataset)
        throw corpus_exception{"dataset missing from configuration file"};

    if(*type == "file-corpus")
    {
        auto file_list = config.get_as<std::string>("list");
        if (!file_list)
            throw corpus_exception{"list missing from configuration file"};

        std::string file = *prefix + "/"
            + *dataset + "/" + *file_list + "-full-corpus.txt";
        return make_unique<file_corpus>(*prefix + "/" + *dataset + "/", file);
    }
    else if(*type == "line-corpus")
    {
        std::string filename = *prefix + "/"
            + *dataset + "/" + *dataset + ".dat";
        auto lines = config.get_as<int64_t>("num-lines");
        if(!lines)
            return make_unique<line_corpus>(filename);
        return make_unique<line_corpus>(filename,
                                        static_cast<uint64_t>(*lines));
    }
    else
        throw corpus_exception{"corpus type was not able to be determined"};
}

}
}
