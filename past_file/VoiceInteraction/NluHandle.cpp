#include "NluHandle.hpp"
#include <numeric> // 

static int argmax(const float* data, size_t size) {
    return std::distance(data, std::max_element(data, data + size));
}

NLUEngine::NLUEngine(const string& modelDir, int maxLen) 
    : maxLen(maxLen), env(ORT_LOGGING_LEVEL_WARNING, "NLU_Engine") {
    
    this->modelPath = modelDir + "/nlu_model.onnx";
    this->vocabPath = modelDir + "/vocab.txt";
    this->metaPath = modelDir + "/meta.json";

}

bool NLUEngine::loadVocab() {
    ifstream file(vocabPath);
    if (!file.is_open()) return false;
    
    string line;
    int idx = 0;
    vocab.clear();
    id2word.clear();
    
    while (getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        vocab[line] = idx;
        id2word.push_back(line);
        idx++;
    }
    
    if (vocab.count("[CLS]")) clsTokenId = vocab["[CLS]"];
    if (vocab.count("[SEP]")) sepTokenId = vocab["[SEP]"];
    if (vocab.count("[PAD]")) padTokenId = vocab["[PAD]"];
    if (vocab.count("[UNK]")) unkTokenId = vocab["[UNK]"];
    
    return true;
}

bool NLUEngine::init() {
    cout << "[Init] Loading Vocab..." << endl;
    if (!loadVocab()) {
        cerr << "Error: Cannot load vocab from " << vocabPath << endl;
        return false;
    }

    cout << "[Init] Loading Meta..." << endl;
    ifstream f(metaPath);
    if (!f.is_open()) {
        cerr << "Error: Cannot load meta from " << metaPath << endl;
        return false;
    }
    json meta;
    try {
        f >> meta;
        for (const auto& l : meta["intents"]) intentLabels.push_back(l);
        for (const auto& l : meta["slots"]) slotLabels.push_back(l);
    } catch (const exception& e) {
        cerr << "JSON Error: " << e.what() << endl;
        return false;
    }

    cout << "[Init] Loading ONNX Model via ONNX Runtime..." << endl;
    try {
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1); 
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

        session = std::make_unique<Ort::Session>(env, modelPath.c_str(), session_options);
    } catch (const Ort::Exception& e) {
        cerr << "ORT Error: " << e.what() << endl;
        return false;
    }

    return true;
}

vector<int> NLUEngine::tokenize(const string& text) {
    
    vector<int> ids;
    ids.push_back(clsTokenId);
    stringstream ss(text);
    string word;
    while (ss >> word) {
        transform(word.begin(), word.end(), word.begin(), ::tolower);
        int start = 0;
        while (start < word.length()) {
            int end = word.length();
            string sub = "";
            bool found = false;
            while (start < end) {
                sub = word.substr(start, end - start);
                if (start > 0) sub = "##" + sub;
                if (vocab.count(sub)) {
                    ids.push_back(vocab[sub]);
                    start = end;
                    found = true;
                    break;
                }
                end--;
            }
            if (!found) {
                ids.push_back(unkTokenId);
                start++;
            }
        }
    }
    ids.push_back(sepTokenId);
    while (ids.size() < maxLen) ids.push_back(padTokenId);
    if (ids.size() > maxLen) {
        ids.resize(maxLen); 
        ids.back() = sepTokenId;
    }
    return ids;
}

nlu_output NLUEngine::predict(const string& text) {

    if (session == nullptr) {
        std::cerr << "[Fatal Error] ORT Session is NULL! Model not loaded." << std::endl;
        return {"Error", "SessionNull", ""};
    }
    //Tokenize
    vector<int> tokenIdsInt = tokenize(text);
    
    
    vector<int64_t> inputIds(tokenIdsInt.begin(), tokenIdsInt.end());
    vector<int64_t> inputMask(maxLen);
    for(int i=0; i<maxLen; i++) inputMask[i] = (inputIds[i] != padTokenId) ? 1 : 0;

    vector<int64_t> inputShape = {1, maxLen};
    
    auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    
    Ort::Value inputIdsTensor = Ort::Value::CreateTensor<int64_t>(
        memoryInfo, inputIds.data(), inputIds.size(), inputShape.data(), inputShape.size());
        
    Ort::Value inputMaskTensor = Ort::Value::CreateTensor<int64_t>(
        memoryInfo, inputMask.data(), inputMask.size(), inputShape.data(), inputShape.size());

    vector<const char*> inputNames = {"input_ids", "attention_mask"};
    vector<const char*> outputNames = {"intent_logits", "slot_logits"};
    
    vector<Ort::Value> inputTensors;
    inputTensors.push_back(std::move(inputIdsTensor));
    inputTensors.push_back(std::move(inputMaskTensor));

    try {
        auto outputTensors = session->Run(
            Ort::RunOptions{nullptr},
            inputNames.data(),
            inputTensors.data(),
            2,
            outputNames.data(),
            2
        );

        // get Intent
        float* intentLogits = outputTensors[0].GetTensorMutableData<float>();
        int intentIdx = argmax(intentLogits, intentLabels.size());
        string intent = intentLabels[intentIdx];

        // get Slots
        float* slotLogits = outputTensors[1].GetTensorMutableData<float>();
        int numSlots = slotLabels.size();

        nlu_output nlu_outp;

        nlu_outp.intent = intent;

        string currentType = "";
        string currentValue = "";
        
        for (int i = 1; i < maxLen; i++) {
            if (inputIds[i] == sepTokenId || inputIds[i] == padTokenId) break;

            float* tokenScores = slotLogits + (i * numSlots);
            int maxSlotIdx = argmax(tokenScores, numSlots);
            
            string tag = slotLabels[maxSlotIdx];
            string word = id2word[tokenIdsInt[i]]; 

            if (tag.rfind("B-", 0) == 0) { 
                currentType = tag.substr(2);
                currentValue = word;
            } else if (tag.rfind("I-", 0) == 0 && currentType == tag.substr(2)) {
                if (word.rfind("##", 0) == 0) currentValue += word.substr(2);
                else currentValue += " " + word;
            } else {
                currentType = "";
                currentValue = "";
            }
        }
        
        nlu_outp.currentType = currentType;
        nlu_outp.currentValue = currentValue;

        return nlu_outp;

    } catch (const Ort::Exception& e) {
        nlu_output nlu_outp = {"None","None","None"};
        cerr << "Inference Error: " << e.what() << endl;
        return nlu_outp;
    }
}

// int main() {

//     string nlu_model_p = "../model";

//     nlu_output nlu_outp;

//     //Init
//     NLUEngine nlu(nlu_model_p); 
//     if (!nlu.init()) return -1;

//     vector<string> test_cases = {
//         "this is an apple",
//         "help me find my phone",
//         "say my name",
//         "what is your name",
//         "i can teach you dance",
//         "do a dance"
//     };

//     for (const auto& text : test_cases) {
//         nlu_outp = nlu.predict(text);
//         cout << "user_command:" << text << endl;
//         cout << "intent:" << nlu_outp.intent << endl;
//         //cout << "MotionName:" << nlu_outp.currentType << endl;
//         cout << "MotionVal:" << nlu_outp.currentValue << endl;
//     }
//     return 0;
// }
