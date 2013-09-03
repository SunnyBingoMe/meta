/**
 * @file disk_index.h
 * @author Sean Massung
 *
 * All files in META are released under the MIT license. For more details,
 * consult the file LICENSE in the root of the project.
 */

#ifndef _DISK_INDEX_H_
#define _DISK_INDEX_H_

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "util/splay_cache.h"
#include "tokenizers/all.h"
#include "index/document.h"
#include "index/postings_data.h"
#include "io/mmap_file.h"
#include "meta.h"

namespace meta {
namespace index {

/**
 * Contains functionality common to inverted_index and forward_index; mainly,
 * creating chunks and merging them together and storing various mappings.
 */
template <class PrimaryKey, class SecondaryKey>
class disk_index
{
    protected:
        /**
         * @param config The toml_group that specifies how to create the
         * index.
         * @param index_path The location to store the index on disk.
         */
        disk_index(const cpptoml::toml_group & config,
                   const std::string & index_path);

        /**
         * Move constructs a disk_index.
         * @param other The disk_index to move into this one.
         */
        disk_index(disk_index && other) = default;

        /**
         * Move assigns a disk_index.
         * @param other The disk_index to move into this one.
         */
        disk_index & operator=(disk_index && other) = default;

        /**
         * disk_index may not be copy-constructed.
         */
        disk_index(const disk_index &) = delete;

        /**
         * disk_index may not be copy-assigned.
         */
        disk_index & operator=(const disk_index &) = delete;

    public:
        /**
         * Default destructor.
         */
        virtual ~disk_index() = default;

        /**
         * @return the name of this index
         */
        std::string index_name() const;

        /**
         * @return the number of documents in this index
         */
        uint64_t num_docs() const;

        /**
         * @param doc_id
         * @return the actual name of this document
         */
        std::string doc_name(doc_id d_id) const;

        /**
         * @param doc_id
         * @return the path to the file containing this document
         */
        std::string doc_path(doc_id d_id) const;

        /**
         * @return a vector of doc_ids that are contained in this index
         */
        std::vector<doc_id> docs() const;

        /**
         * @param d_id The document to search for
         * @return the size of the given document (the total number of terms
         * occuring)
         */
        uint64_t doc_size(doc_id d_id) const;

        /**
         * @param doc The document to tokenize
         */
        void tokenize(document & doc);

        /**
         * @param d_id The doc id to find the class label for
         * @return the label of the class that the document belongs to, or an
         * empty string if a label was not assigned
         */
        class_label label(doc_id d_id) const;

        /**
         * @param l_id The id of the class label in question
         * @return the integer label id of a document
         */
        class_label class_label_from_id(label_id l_id) const;

        /**
         * @param d_id
         * @return the number of unique terms in d_id
         */
        uint64_t unique_terms(doc_id d_id) const;

    protected:
        /**
         * This function initializes the disk index. It cannot be part of the
         * constructor since dynamic binding doesn't work in a base class's
         * constructor, so it is invoked from a factory method.
         * @param docs The documents to tokenize
         * @param config_file The configuration file used to create the
         */
        void create_index(std::vector<document> & docs,
                          const std::string & config_file);

        /**
         * This function loads a disk index from its filesystem
         * representation.
         */
        void load_index();

        /**
         * @param p_id The PrimaryKey id to search for
         * @return the postings data for a given PrimaryKey
         * A cache is first searched before the postings file is queried.
         */
        postings_data<PrimaryKey, SecondaryKey>
        search_primary(PrimaryKey p_id) const;

        /**
         * @param chunk_num The current chunk number of the postings file
         * @param pdata A collection of postings data to write to the chunk
         */
        void write_chunk(uint32_t chunk_num,
                         std::unordered_map<
                            PrimaryKey,
                            postings_data<PrimaryKey, SecondaryKey>
                         > & pdata);

        /**
         * Saves any arbitrary mapping to the disk.
         * @param filename The name to save the mapping as
         */
        template <class Key, class Value>
        void save_mapping(const std::unordered_map<Key, Value> & map,
                          const std::string & filename) const;

        /**
         * @param map The map to load information into
         * @param filename The file containing key, value pairs
         */
        template <class Key, class Value>
        void load_mapping(std::unordered_map<Key, Value> & map,
                          const std::string & filename);

        /**
         * @param docs The documents to add to the inverted index
         */
        virtual uint32_t tokenize_docs(std::vector<document> & docs) = 0;

        /**
         * @param d_id The document
         * @return the numerical label_id for a given document's label
         */
        label_id label_id_from_doc(doc_id d_id) const;

        /** doc_id -> document path mapping */
        std::unordered_map<doc_id, std::string> _doc_id_mapping;

        /** doc_id -> document length mapping */
        std::unordered_map<doc_id, uint64_t> _doc_sizes;

        /** the tokenizer used to tokenize documents in the index */
        std::unique_ptr<tokenizers::tokenizer> _tokenizer;

    private:
        /**
         * @param num_chunks The total number of chunks to merge together to
         * create the postings file
         * @param filename The name for the postings file
         */
        void merge_chunks(uint32_t num_chunks, const std::string & filename);

        /**
         * @param idx The pointer into the postings file where the wanted
         * PrimaryKey begins
         * @return a postings_data object from the postings file
         */
        postings_data<PrimaryKey, SecondaryKey>
        search_postings(uint64_t idx) const;

        /**
         * Creates the lexicon file (or "dictionary") which has pointers into
         * the large postings file
         * @param postings_file
         * @param lexicon_file
         */
        void create_lexicon(const std::string & postings_file,
                            const std::string & lexicon_file);

        /**
         * Initializes the _label_ids member.
         */
        void set_label_ids();

        /** the location of this index */
        std::string _index_name;

        /** PrimaryKey -> postings location */
        std::unordered_map<PrimaryKey, uint64_t> _term_locations;

        /**
         * A pointer to a memory-mapped postings file. It is a pointer because
         * we want to delay the initialization of it until the postings file is
         * created in some cases.
         */
        std::unique_ptr<io::mmap_file> _postings;

        /** cache for recently used postings_data */
        mutable util::splay_cache<
            PrimaryKey, postings_data<PrimaryKey, SecondaryKey>
        > _cache;

        /** mutex used when the cache is accessed */
        mutable std::unique_ptr<std::mutex> _mutex;

        /** maps which class a document belongs to (if any) */
        std::unordered_map<doc_id, class_label> _labels;

        /** assigns an integer to each class label (used for liblinear and slda
         * mappings) */
        util::invertible_map<class_label, label_id> _label_ids;

        /**
         * Holds how many unique terms there are per-document. This is sort of
         * like an inverse IDF. For a forward_index, this field is certainly
         * redundant, though it can save querying the postings file.
         */
        std::unordered_map<doc_id, uint64_t> _unique_terms;

    public:
        /**
         * Basic exception for disk_index interactions.
         */
        class disk_index_exception: public std::exception
        {
            public:

                disk_index_exception(const std::string & error):
                    _error(error) { /* nothing */ }

                const char* what () const throw ()
                {
                    return _error.c_str();
                }

            private:
                std::string _error;
        };

        /**
         * Factory method for creating indexes.
         * Usage:
         *
         * ~~~cpp
         * auto idx = index::make_index<derived_index_type>(config_path);
         * ~~~
         *
         * @param config_file The path to the configuration file to be
         * used to build the index
         * @return A properly initialized index
         */
        template <class Index>
        friend Index make_index(const std::string & config_file);
};

}
}

#include "index/disk_index.tcc"
#endif