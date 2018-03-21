#ifndef QTUMTRANSACTION_H
#define QTUMTRANSACTION_H

#include <libethcore/Transaction.h>
#include <base58.h>

struct VersionVM{
    //this should be portable, see https://stackoverflow.com/questions/31726191/is-there-a-portable-alternative-to-c-bitfields
# if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8_t format : 2;
    uint8_t rootVM : 6;
#elif __BYTE_ORDER == __BIG_ENDIAN
    uint8_t rootVM : 6;
    uint8_t format : 2;
#endif
    uint8_t vmVersion;
    uint16_t flagOptions;
    // CONSENSUS CRITICAL!
    // Do not add any other fields to this struct

    uint32_t toRaw(){
        return *(uint32_t*)this;
    }
    static VersionVM fromRaw(uint32_t val){
        VersionVM x = *(VersionVM*)&val;
        return x;
    }
    static VersionVM GetNoExec(){
        VersionVM x;
        x.flagOptions=0;
        x.rootVM=0;
        x.format=0;
        x.vmVersion=0;
        return x;
    }
    static VersionVM GetEVMDefault(){
        VersionVM x;
        x.flagOptions=0;
        x.rootVM=1;
        x.format=0;
        x.vmVersion=0;
        return x;
    }
}__attribute__((__packed__));


class QtumEthTransaction : public dev::eth::Transaction{

public:

    QtumEthTransaction() : nVout(0) {}

    QtumEthTransaction(dev::u256 const& _value, dev::u256 const& _gasPrice, dev::u256 const& _gas, dev::bytes const& _data, dev::u256 const& _nonce = dev::Invalid256):
            dev::eth::Transaction(_value, _gasPrice, _gas, _data, _nonce) {}

    QtumEthTransaction(dev::u256 const& _value, dev::u256 const& _gasPrice, dev::u256 const& _gas, dev::Address const& _dest, dev::bytes const& _data, dev::u256 const& _nonce = dev::Invalid256):
            dev::eth::Transaction(_value, _gasPrice, _gas, _dest, _data, _nonce) {}

    void setHashWith(const dev::h256 hash) { m_hashWith = hash; }

    dev::h256 getHashWith() const { return m_hashWith; }

    void setNVout(uint32_t vout) { nVout = vout; }

    uint32_t getNVout() const { return nVout; }

    void setVersion(VersionVM v){
        version=v;
    }
    VersionVM getVersion() const{
        return version;
    }
private:

    uint32_t nVout;
    VersionVM version;

};


class QtumTransaction{

public:

    QtumTransaction(uint64_t _value, uint64_t _gasPrice, uint64_t _gas, std::vector<uint8_t> const& _data){
        hasDestination = false;
        create = false;
        value = _value;
        gasPrice = _gasPrice;
        gas = _gas;
        data = _data;
    }

    QtumTransaction(uint64_t _value, uint64_t _gasPrice, uint64_t _gas, std::vector<uint8_t> const& _dest, std::vector<uint8_t> const& _data) {
        hasDestination = true;
        create = false;
        value = _value;
        gasPrice = _gasPrice;
        gas = _gas;
        data = _data;
        destination = _dest;
    }
    void setCreation(bool _create){
        create = _create;
    }
    void setHashWith(const uint256 hash) { txid = hash; }

    uint256 getHashWith() const { return txid; }

    void setNVout(uint32_t vout) { nVout = vout; }

    uint32_t getNVout() const { return nVout; }

    void setVersion(VersionVM v){
        version=v;
    }
    VersionVM getVersion() const{
        return version;
    }

    QtumEthTransaction getEthTx(){
        if(hasDestination){
            QtumEthTransaction tx(dev::u256(value), dev::u256(gasPrice), dev::u256(gas), dev::Address(destination), (dev::bytes)data, dev::Invalid256);
            tx.setNVout(nVout);
            tx.setHashWith(uintToh256(txid));
            tx.setVersion(version);
        }else {
            QtumEthTransaction tx(dev::u256(value), dev::u256(gasPrice), dev::u256(gas), (dev::bytes)data, dev::Invalid256);
            tx.setNVout(nVout);
            tx.setHashWith(uintToh256(txid));
            tx.setVersion(version);
        }
    }
    uint64_t getGas(){
        return gas;
    }
    uint64_t getGasPrice(){
        return gasPrice;
    }
    uint64_t getValue(){
        return value;
    }
private:
    bool hasDestination;
    uint32_t nVout;
    VersionVM version;
    uint64_t value, gasPrice, gas;
    std::vector<uint8_t> destination;
    std::vector<uint8_t> data;
    uint256 txid;
    bool create;

};
#endif
