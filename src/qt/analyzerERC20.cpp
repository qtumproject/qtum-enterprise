#include <analyzerERC20.h>

std::string ERC20 = "[{\"constant\":false,\"inputs\":[{\"name\":\"_spender\",\"type\":\"address\"},{\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"approve\",\"outputs\":[{\"name\":\"success\",\"type\":\"bool\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":true,\"inputs\":[],\"name\":\"totalSupply\",\"outputs\":[{\"name\":\"totalSupply\",\"type\":\"uint256\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":false,\"inputs\":[{\"name\":\"_from\",\"type\":\"address\"},{\"name\":\"_to\",\"type\":\"address\"},{\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"transferFrom\",\"outputs\":[{\"name\":\"success\",\"type\":\"bool\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":true,\"inputs\":[{\"name\":\"_owner\",\"type\":\"address\"}],\"name\":\"balanceOf\",\"outputs\":[{\"name\":\"balance\",\"type\":\"uint256\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":false,\"inputs\":[{\"name\":\"_to\",\"type\":\"address\"},{\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"transfer\",\"outputs\":[{\"name\":\"success\",\"type\":\"bool\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":true,\"inputs\":[{\"name\":\"_owner\",\"type\":\"address\"},{\"name\":\"_spender\",\"type\":\"address\"}],\"name\":\"allowance\",\"outputs\":[{\"name\":\"remaining\",\"type\":\"uint256\"}],\"payable\":false,\"type\":\"function\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"name\":\"_from\",\"type\":\"address\"},{\"indexed\":true,\"name\":\"_to\",\"type\":\"address\"},{\"indexed\":false,\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"Transfer\",\"type\":\"event\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"name\":\"_owner\",\"type\":\"address\"},{\"indexed\":true,\"name\":\"_spender\",\"type\":\"address\"},{\"indexed\":false,\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"Approval\",\"type\":\"event\"}]";

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






std::string ParserAbi::strDecTostrHex(const std::string& str){
    dev::u256 number(str);
    dev::h256 temp(number);
    return temp.hex();
}

void ParserAbi::createInputData(const std::string& methodName, const Parameters& params){
    // std::regex byteNumber("(\d+)");
    // std::regex typeName("([a-z]+)");
    // std::regex intR("\\b(int)");
    // std::regex_match(type, intR)
    Parameters deleteP = {{"uint8","123456789"},{"uint16","987654321"},{"uint32","13"},
                          {"int8","-123456789"},{"int32","987654321"},{"int256","-13"},
                          {"address","ffffffffffffffffffffffffffffffffffffffff"},{"address","aaaaaaffffffffffffffffffffffffffffffffff"},{"address","aaaaaaaaaaaaaaaaaaffffffffffffffffffffff"},
                          {"bool","true"},{"bool","fasle"},{"bool","0"},
                          {"string","true"},{"string","abcdefgklmnoprst"},{"string","abcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprstabcdefgklmnoprst"}};

    std::vector<std::string> result;
    std::vector<std::string> stack;

    // size_t offset = 32 * params.size();
    // for(auto p : params){
    size_t offset = 32 * deleteP.size();
    for(auto p : deleteP){
        std::string type = p.first;

        if(type == "uint8" || type == "uint16" || type == "uint32" || type == "uint64" || type == "uint128" || type == "uint256"){
            
            result.push_back(strDecTostrHex(p.second));
        
        } else if(type == "int8" || type == "int16" || type == "int32" || type == "int64" || type == "int128" || type == "int256"){

            std::string dataInt = p.second[0] == '-' ? std::string(p.second.begin() + 1, p.second.end()) : p.second;
            std::string temp = strDecTostrHex(dataInt);
            if(p.second[0] == '-'){
                temp[0] = 'f';
                temp[1] = 'f';
            }
            result.push_back(temp);

        } else if(type == "address"){

            std::string address(std::string(24, '0') + p.second);
            result.push_back(address);

        } else if(type == "bool"){

            if(p.second == "true" || p.second == "1"){
                result.push_back(std::string(63, '0') + '1');
            } else {
                result.push_back(std::string(64, '0'));
            }

        } else if(type == "string"){

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
                std::stringstream s;
                if(sizeString >= 32){
                    s << std::hex << std::string(p.second.begin() + (32 * i), p.second.begin() + (32 * i) + 32);
                    sizeString -= 32;
                    stack.push_back(s.str());
                } else {
                    s << std::hex << std::string(p.second.begin() + (32 * i), p.second.begin() + (32 * i) + sizeString);
                    stack.push_back(s.str() + std::string(64 - s.str().size(), '0'));
                }
            }
        } else if(type == "uint8[]" || type == "uint16[]" || type == "uint32[]" || type == "uint64[]" || type == "uint128[]" || type == "uint256[]"){
            
            std::string word;
            std::vector<std::string> wordsVector;
            std::istringstream iss(p.second, std::istringstream::in);
            while( iss >> word )     
            {
                wordsVector.push_back(word);
            }
            std::reverse(begin(wordsVector), end(wordsVector));

            if(type == "uint8[]"){

            } else if(type == "uint16[]"){

            } else if(type == "uint32[]"){

            } else if(type == "uint64[]"){

            } else if(type == "uint128[]"){

            } else if(type == "uint256[]"){

            }

        }
    }
    // std::map<string, string> data;
}
