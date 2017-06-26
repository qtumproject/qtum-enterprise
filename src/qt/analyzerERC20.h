#include <univalue.h>
#include "libdevcore/Common.h"
#include "libdevcore/FixedHash.h"
#include <regex>
#include "utilstrencodings.h"

using Parameters = std::vector<std::pair<std::string, std::string>>;
using DataAndStack = std::pair<std::vector<std::string>, std::vector<std::string>>;

extern std::string ERC20;

struct ContractMethodParams{
    bool indexed = false;
    std::string name = "";
    std::string type = "";

    bool operator!=(ContractMethodParams& cmp){
        if(cmp.indexed == indexed && cmp.name == name && cmp.type == type)
            return false;
        return true;
    }
};

struct ContractMethod{
    bool constant = false;
    bool anonymous = false;
    std::vector<ContractMethodParams> inputs;
    std::string name = "";
    std::vector<ContractMethodParams> outputs;
    bool payable = false;
    std::string type = "";

    bool equalContractMethodParams(std::vector<ContractMethodParams>& cmps1, std::vector<ContractMethodParams>& cmps2);
    bool operator!=(ContractMethod& cm);
};

class ParserAbi{

public:

    void parseAbiJSON(const std::string jsonStr);

    std::string createInputData(const std::string& methodName, const Parameters& params);

    bool checkData(std::string data, std::string type);

    std::map<std::string, ContractMethod> getContractMethods() { return contractMethods; }

private:

    std::string strDecToStrHex(const std::string& str);

    std::string stringToStrHex(const std::string& str);

    std::pair<size_t, size_t> updateOffset(size_t offset, size_t size);

    std::string creatingDataFromElementaryTypes(const std::string& type, const std::string& data);

    DataAndStack creatingDataFromStringAndBytes(size_t& offset, const std::string& data);

    DataAndStack creatingDataFromArray(size_t& offset, const std::string& data, std::string& type);

    DataAndStack creatingDataFromArrayToArray(size_t& offset, const std::string& data, std::string& type);

    bool checkElementaryTypes(std::string data, std::string& type);

    bool checkArray(std::string& data, std::string& type);
    
    bool checkArrayToArray(std::string& data, std::string& type);
    
    std::map<std::string, ContractMethod> contractMethods;
};

class AnalyzerERC20{

public:

    AnalyzerERC20() { parser.parseAbiJSON(ERC20); ERC20Methods = parser.getContractMethods(); };

    bool isERC20(const std::string& contractAbi);

private:

    std::map<std::string, ContractMethod> ERC20Methods;

    ParserAbi parser;
};
