#include <analyzerERC20.h>

std::string ERC20 = "[{\"constant\":false,\"inputs\":[{\"name\":\"_spender\",\"type\":\"address\"},{\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"approve\",\"outputs\":[{\"name\":\"success\",\"type\":\"bool\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":true,\"inputs\":[],\"name\":\"totalSupply\",\"outputs\":[{\"name\":\"totalSupply\",\"type\":\"uint256\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":false,\"inputs\":[{\"name\":\"_from\",\"type\":\"address\"},{\"name\":\"_to\",\"type\":\"address\"},{\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"transferFrom\",\"outputs\":[{\"name\":\"success\",\"type\":\"bool\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":true,\"inputs\":[{\"name\":\"_owner\",\"type\":\"address\"}],\"name\":\"balanceOf\",\"outputs\":[{\"name\":\"balance\",\"type\":\"uint256\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":false,\"inputs\":[{\"name\":\"_to\",\"type\":\"address\"},{\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"transfer\",\"outputs\":[{\"name\":\"success\",\"type\":\"bool\"}],\"payable\":false,\"type\":\"function\"},{\"constant\":true,\"inputs\":[{\"name\":\"_owner\",\"type\":\"address\"},{\"name\":\"_spender\",\"type\":\"address\"}],\"name\":\"allowance\",\"outputs\":[{\"name\":\"remaining\",\"type\":\"uint256\"}],\"payable\":false,\"type\":\"function\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"name\":\"_from\",\"type\":\"address\"},{\"indexed\":true,\"name\":\"_to\",\"type\":\"address\"},{\"indexed\":false,\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"Transfer\",\"type\":\"event\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"name\":\"_owner\",\"type\":\"address\"},{\"indexed\":true,\"name\":\"_spender\",\"type\":\"address\"},{\"indexed\":false,\"name\":\"_value\",\"type\":\"uint256\"}],\"name\":\"Approval\",\"type\":\"event\"}]";

bool AnalyzerERC20::isERC20(const std::string& contractAbi){
    contractMethods = parseAbiJSON(contractAbi);
    for(std::pair<std::string, ContractMethod> cm : ERC20Methods){
        if((contractMethods.count(cm.first) && (contractMethods[cm.first] != cm.second)) || !contractMethods.count(cm.first))
            return false;
    }
    return true;
}

std::map<std::string, ContractMethod> AnalyzerERC20::parseAbiJSON(const std::string jsonStr){
    std::map<std::string, ContractMethod> result;
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
            result[method.name] = method;
        }
    }
    return result;
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
