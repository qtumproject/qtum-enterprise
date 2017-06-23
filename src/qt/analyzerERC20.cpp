#include <analyzerERC20.h>

std::string ERC20 = "[{\"constant\":false,\"inputs\":[{\"name\":\"_spender\",\"type\":\"address\"},{\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"approve\",\"outputs\":[{\"name\":\"success\",\"type\":\"bool\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":true,\"inputs\":[],\"name\":\"totalSupply\",\"outputs\":[{\"name\":\"totalSupply\",\"type\":\"uint256\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":false,\"inputs\":[{\"name\":\"_from\",\"type\":\"address\"},{\"name\":\"_to\",\"type\":\"address\"},{\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"transferFrom\",\"outputs\":[{\"name\":\"success\",\"type\":\"bool\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":true,\"inputs\":[{\"name\":\"_owner\",\"type\":\"address\"}],\"name\":\"balanceOf\",\"outputs\":[{\"name\":\"balance\",\"type\":\"uint256\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":false,\"inputs\":[{\"name\":\"_to\",\"type\":\"address\"},{\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"transfer\",\"outputs\":[{\"name\":\"success\",\"type\":\"bool\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":true,\"inputs\":[{\"name\":\"_owner\",\"type\":\"address\"},{\"name\":\"_spender\",\"type\":\"address\"}],\"name\":\"allowance\",\"outputs\":[{\"name\":\"remaining\",\"type\":\"uint256\"}],\"payable\":false,\"type\":\"function\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"name\":\"_from\",\"type\":\"address\"},{\"indexed\":true,\"name\":\"_to\",\"type\":\"address\"},{\"indexed\":false,\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"Transfer\",\"type\":\"event\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"name\":\"_owner\",\"type\":\"address\"},{\"indexed\":true,\"name\":\"_spender\",\"type\":\"address\"},{\"indexed\":false,\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"Approval\",\"type\":\"event\"}]";

std::regex uintRegex("(uint[0-9]{0,3})");
std::regex intRegex("(int[0-9]{0,3})");
std::regex bytesNRegex("(bytes[0-9]{1,3})");
std::regex bytesRegex("(bytes)");
// std::regex masRegex("^([a-zA-Z]\\w+\\[\\])$");
std::regex masRegex("^([a-zA-Z]\\w+\\[[0-9+]{0,}\\])$");
std::regex masNumElementRegex("\\[([1-9]+)\\]$");

void ParserAbi::parseAbiJSON(const std::string jsonStr){
    contractMethods.clear();
    if(!jsonStr.empty()){
        UniValue contract(UniValue::VARR);
        contract.read(jsonStr.data());

        for(size_t i = 0; i < contract.size(); i++){
            ContractMethod method;
            UniValue obj(UniValue::VOBJ);
            obj = contract[i].get_obj();

            std::vector<std::string> keysObj = obj.getKeys();
            for(size_t k = 0; k < keysObj.size(); k++){
                if(keysObj[k] == "constant") {
                    method.constant = obj[k].get_bool();
                } else if(keysObj[k] == "anonymous") {
                    method.anonymous = obj[k].get_bool();
                } else if(keysObj[k] == "inputs") {
                    UniValue inputs(UniValue::VARR);
                    inputs = obj[k].get_array();
                    for(size_t j = 0; j < inputs.size(); j++){
                        ContractMethodParams params;
                        if(inputs[j].size() > 2){
                            params.indexed = inputs[j][0].get_bool();
                            params.name = inputs[j][1].get_str();
                            params.type = inputs[j][2].get_str();
                        } else {
                            params.name = inputs[j][0].get_str();
                            params.type = inputs[j][1].get_str();
                        }
                        method.inputs.push_back(params);
                    }
                } else if(keysObj[k] == "name") {
                    method.name = obj[k].get_str();
                } else if(keysObj[k] == "outputs") {
                    UniValue outputs(UniValue::VARR);
                    outputs = obj[k].get_array();
                    for(size_t j = 0; j < outputs.size(); j++){
                        ContractMethodParams params;
                        params.name = outputs[j][0].get_str();
                        params.type = outputs[j][1].get_str();
                        method.outputs.push_back(params);
                    }
                } else if(keysObj[k] == "payable") {
                    method.payable = obj[k].get_bool();
                } else if(keysObj[k] == "type") {
                    method.type = obj[k].get_str();
                }
            }
            contractMethods[method.name] = method;
        }
    }
}

bool AnalyzerERC20::isERC20(const std::string& contractAbi){
    parser.parseAbiJSON(contractAbi); 
    std::map<std::string, ContractMethod> contractMethods = parser.getContractMethods();

    for(std::pair<std::string, ContractMethod> cm : ERC20Methods){
        if((contractMethods.count(cm.first) && (contractMethods[cm.first] != cm.second)) || !contractMethods.count(cm.first))
            return false;
    }
    return true;
}

bool ContractMethod::equalContractMethodParams(std::vector<ContractMethodParams>& cmps1, std::vector<ContractMethodParams>& cmps2){
    if(cmps1.size() == cmps2.size()){
        for(size_t i = 0; i < cmps1.size(); i++){
            if(cmps1[i] != cmps2[i])
                return false;
        }
        return true;
    }
    return false;
}

bool ContractMethod::operator!=(ContractMethod& cm){
    if(cm.constant == constant && cm.constant == constant && equalContractMethodParams(cm.inputs, inputs) &&
       cm.name == name && equalContractMethodParams(cm.outputs, outputs) && cm.payable == payable && cm.type == type){
            return false;
    }
    return true;
}






std::string ParserAbi::strDecToStrHex(const std::string& str){
    dev::u256 number(str);
    dev::h256 temp(number);
    return temp.hex();
}

std::string ParserAbi::stringToStrHex(const std::string& str){
    std::vector<unsigned char> bytes(str.begin(), str.end());
    return HexStr(bytes.begin(), bytes.end());
}

std::string ParserAbi::creatingDataFromElementaryTypes(const std::string& type, const std::string& data){

    if(std::regex_match(type, uintRegex)){
        return strDecToStrHex(data);
    } else if(std::regex_match(type, intRegex)){
        std::string dataInt = data[0] == '-' ? std::string(data.begin() + 1, data.end()) : data;
        std::string temp = strDecToStrHex(dataInt);
        if(data[0] == '-'){
            temp[0] = 'f';
            temp[1] = 'f';
        }
        return temp;
    } else if(std::regex_match(type, bytesNRegex)){
        std::string dataHex = stringToStrHex(data);
        return std::string(dataHex + std::string(64 - dataHex.size(), '0'));
    } else if(type == "bool"){
        if(data == "true" || data == "1"){
            return std::string(63, '0') + '1';
        } else {
            return std::string(64, '0');
        }
    } else {
        std::string address(std::string(24, '0') + data);
        return address;
    }
}

void ParserAbi::createInputData(const std::string& methodName, const Parameters& params){

    Parameters deleteP = {{"uint8","123456789"},{"uint16","987654321"},{"uint32","13"},
                          {"int8","-123456789"},{"int32","987654321"},{"int256","-13"},
                          {"address","ffffffffffffffffffffffffffffffffffffffff"},{"address","aaaaaaffffffffffffffffffffffffffffffffff"},{"address","aaaaaaaaaaaaaaaaaaffffffffffffffffffffff"},
                          {"bool","true"},{"bool","fasle"},{"bool","0"},
                          {"string","a"},{"string","abcdefgklmnoprst"},{"string","abcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprst"},
                          {"bytes","1234567890"},{"bytes","Hello, world!"},{"bytes","abcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprst"},
                          {"bytes1","1"},{"bytes13","Hello, world!"},{"bytes32","abcdefgklmnoprstabcdefgklmnoprst"},
                          {"uint8[]","13"},{"bytes[5]","Hello, world!"},{"ds43ds5534a5354das[]","abcdefgklmnoprstabcdefgklmnoprst"}, {"ds43ds5534a5354das[][]","abcdefgklmnoprstabcdefgklmnoprst"}};

    
    
    std::vector<std::string> result;
    std::vector<std::string> stack;

    // size_t offset = 32 * params.size();
    // for(auto p : params){
    size_t offset = 32 * deleteP.size();
    for(auto p : deleteP){
        std::string type = p.first;

        if(std::regex_match(type, uintRegex) || std::regex_match(type, intRegex) || 
           std::regex_match(type, bytesNRegex) || type == "address" || type == "bool"){

            result.push_back(creatingDataFromElementaryTypes(type, p.second));
            
        } else if(type == "string" || std::regex_match(type, bytesRegex)){

            // update offset
            size_t multiplier = 3; // str(offset) + str(len) + str(text)
            size_t sizeString = p.second.size();
            if(sizeString > 32){
                size_t fullLines = sizeString / 32;
                multiplier += fullLines;
            }
            offset += 32 * multiplier;

            std::stringstream ss;
            ss << std::hex << offset;
            result.push_back(std::string(64 - ss.str().size(), '0') + ss.str());

            std::stringstream sss;
            sss << std::hex << p.second.size();
            stack.push_back(std::string(64 - sss.str().size(), '0') + sss.str());

            for(size_t i = 0; i < multiplier - 2; i++){
                std::string s;
                if(sizeString >= 32){
                    s = stringToStrHex(std::string(p.second.begin() + (32 * i), p.second.begin() + (32 * i) + 32));
                    stack.push_back(s);
                    sizeString -= 32;
                } else {
                    s = stringToStrHex(std::string(p.second.begin() + (32 * i), p.second.begin() + (32 * i) + sizeString));
                    stack.push_back(s + std::string(64 - s.size(), '0'));
                }
            }
        } else if(std::regex_match(type, masRegex)){

            std::string tempType;
            std::smatch m;
            std::regex typeRegex("([a-zA-Z]\\w+)");
            if(std::regex_search (type, m, typeRegex)){
                tempType = m[0].str();
            }

            std::regex regex("(\\w+)");
            size_t numElem;
            std::smatch r;
            if(std::regex_search(type, r, masNumElementRegex)){

                numElem = std::stoi(r[1].str());
                
                std::sregex_token_iterator it(begin(p.second), end(p.second), regex), last;
                for (size_t i = 0; i < numElem; i++){
                    if(it != last){
                        result.push_back(creatingDataFromElementaryTypes(tempType, it->str()));
                        ++it;
                    }
                }
            } else {
                for (std::sregex_token_iterator it(begin(p.second), end(p.second), regex), last; it != last; ++it)
                    result.push_back(creatingDataFromElementaryTypes(tempType, it->str()));
            }
        }
    }
    // std::map<string, string> data;
}
