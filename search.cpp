/**
 * @file search.cpp
 */

#include <utility>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#include "io/config_reader.h"
#include "tokenizers/tokenizer.h"
#include "index/document.h"
#include "cluster/similarity.h"

using std::pair;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::unordered_map;

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        cerr << "Usage:\t" << argv[0] << " configFile" << endl;
        return 1;
    }

    unordered_map<string, string> config = ConfigReader::read(argv[1]);
    string prefix = "/home/sean/projects/senior-thesis-data/" + config["prefix"];

    std::shared_ptr<Tokenizer> tokenizer = ConfigReader::create_tokenizer(config);

    vector<Document> docs = Document::loadDocs(prefix + "/full-corpus.txt", prefix);

    cerr << "Tokenizing..." << endl;
    for(auto & doc: docs)
        tokenizer->tokenize(doc);

    cerr << "Computing similarities..." << endl;
    vector<pair<string, double>> scores;
    scores.reserve(docs.size() * docs.size());
    for(size_t i = 0; i < docs.size(); ++i)
    {
        cerr << "  " << docs.size() - i - 1 << " remaining    \r";
        for(size_t j = 0; j < i; ++j)
        {
            string comp = docs[i].getName() + " " + docs[j].getName();
            // heuristically guess if the assignment is completed
            if(docs[i].getLength() > 10 && docs[j].getLength() > 10)
                scores.push_back(make_pair(comp, Document::cosine_similarity(docs[i], docs[j])));
        }
    }
    cerr << endl;

    std::sort(scores.begin(), scores.end(), [](const pair<string, double> & a, const pair<string, double> & b){
        return a.second > b.second;
    });

    for(size_t i = 0; i < scores.size(); ++i)
        cout << scores[i].second << " " << scores[i].first << endl;

    return 0;
}