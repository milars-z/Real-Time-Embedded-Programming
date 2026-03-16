#ifndef HLUHANDLE_HPP
#define HLUHANDLE_HPP

#include <onnxruntime_cxx_api.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <sstream>
#include <memory> // for std::unique_ptr
#include <nlohmann/json.hpp> 

using json = nlohmann::json;
using namespace std;

struct nlu_output {
    string currentType;                
    string currentValue;
    string intent;
};

class NLUEngine {
public:
    NLUEngine(const string& modelDir, int maxLen = 32);
    bool init();
    nlu_output predict(const string& text);

private:

   
    Ort::Env env;
    std::unique_ptr<Ort::Session> session; 

    string modelPath;
    string vocabPath;
    string metaPath;
    int maxLen;



    map<string, int> vocab;
    vector<string> id2word;
    vector<string> intentLabels;
    vector<string> slotLabels;

    int clsTokenId = 101;
    int sepTokenId = 102;
    int padTokenId = 0;
    int unkTokenId = 100;

    bool loadVocab();
    vector<int> tokenize(const string& text);
};

#endif // NLU_TEST_HPP