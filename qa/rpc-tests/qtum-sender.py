#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.blocktools import *
from test_framework.qtum import *

class QtumSenderTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 1
        self.setup_clean_chain = True

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-txindex=1']])
        self.is_network_split = False
        self.node = self.nodes[0]

    def setup_address_contract(self):
        """
        contract SetSenderTest {
            address public sender;
            address public origin;

            function SetSenderTest() {
                sender = 0x1111111111111111111111111111111111111111;
                origin = 0x1111111111111111111111111111111111111111;
            }

            function() payable {
                sender = msg.sender;
                origin = tx.origin;
            }
        }
       """
        bytecode = "6060604052341561000f57600080fd5b7311111111111111111111111111111111111111116000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550731111111111111111111111111111111111111111600160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055506101f0806100c76000396000f30060606040526004361061004c576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806367e404ce146100cf578063938b5f3214610124575b336000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555032600160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550005b34156100da57600080fd5b6100e2610179565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b341561012f57600080fd5b61013761019e565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b6000809054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b600160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16815600a165627a7a72305820d84f9d62f00d47e3dab1e1f2ffb286995557cf0a13ef87e952f455dc84656b860029"
        self.contract_address = self.node.createcontract(bytecode)['address']
        self.node.generate(1)

    def setup_dummy_contract(self):
        """
        contract Test {
            function() payable {}
        }
       """
        bytecode = "60606040523415600e57600080fd5b603280601b6000396000f30060606040520000a165627a7a7230582083f4e618ca67012e4b0bf13057bb820c0c34cd93bd1a65ca969065cbf89f726c0029"
        self.contract_address = self.node.createcontract(bytecode)['address']
        self.node.generate(1)

    def setup_proxy_contract(self):
        """
        contract ProxyTest {
            function proxy(address other) payable {
                other.call(0x0);
            }
        }
       """
        bytecode = "60606040523415600e57600080fd5b60de8061001c6000396000f300606060405260043610603f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806306713c3e146044575b600080fd5b606e600480803573ffffffffffffffffffffffffffffffffffffffff169060200190919050506070565b005b8073ffffffffffffffffffffffffffffffffffffffff166000604051808260ff16815260200191505060006040518083038160008661646e5a03f191505050505600a165627a7a723058203b1a21fc74db82f75856b1371fdbdc0e55b38fce603fc60fb5e73629f570cf020029"
        self.proxy_contract_address = self.node.createcontract(bytecode)['address']
        self.node.generate(1)

    def assert_sender(self, expected_sender='1111111111111111111111111111111111111111', expected_origin='1111111111111111111111111111111111111111'):
        # call sender()
        ret = self.node.callcontract(self.contract_address, "67e404ce")['executionResult']['output']
        assert_equal(ret, expected_sender.zfill(64))

        # call origin()
        ret = self.node.callcontract(self.contract_address, "938b5f32")['executionResult']['output']
        assert_equal(ret, expected_origin.zfill(64))

    def make_spendable_tx(self, pubkey):
        unspent = self.unspents.pop(0)
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(unspent['txid'], 16), unspent['vout']))]
        tx.vout = [CTxOut(1999999900000, scriptPubKey=CScript(pubkey))]
        tx = rpc_sign_transaction(self.node, tx)
        return tx

    def send_spendable_tx(self, pubkey):
        tx = self.make_spendable_tx(pubkey)
        txid = self.node.sendrawtransaction(bytes_to_hex_str(tx.serialize()))
        return COutPoint(int(txid, 16), 0)

    def make_contract_tx_spending_outpoint(self, outpoint):
        tx = CTransaction()
        tx.vin = [CTxIn(outpoint)]
        tx.vout = [CTxOut(1999900000000, scriptPubKey=CScript([b"\x04", 1000000, QTUM_MIN_GAS_PRICE, b"\x00", hex_str_to_bytes(self.contract_address), OP_CALL]))]
        return tx

    def create_block_with_txes(self, txs):
        tip = self.node.getblock(self.node.getbestblockhash())
        block = create_block(int(tip['hash'], 16), create_coinbase(tip['height']+1), tip['time']+1)
        block.hashStateRoot = int(tip['hashStateRoot'], 16)
        block.hashUTXORoot = int(tip['hashUTXORoot'], 16)
        block.vtx.extend(txs)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        return block

    def verify_non_standard_sender_test(self):
        self.setup_dummy_contract()
        outpoint = self.send_spendable_tx([OP_TRUE])
        tx = self.make_contract_tx_spending_outpoint(outpoint)
        assert_raises_message(JSONRPCException, "bad-txns-invalid-sender-script", self.node.sendrawtransaction, bytes_to_hex_str(tx.serialize()))
        self.node.generate(1)

        # Do the same test but make sure that a block containing such a tx is rejected
        contract_tx = self.make_contract_tx_spending_outpoint(outpoint)
        block = self.create_block_with_txes([contract_tx])
        old_block_count = self.node.getblockcount()
        self.node.submitblock(bytes_to_hex_str(block.serialize()))
        assert_equal(old_block_count, self.node.getblockcount())

    def verify_p2sh_sender_test(self):
        self.setup_dummy_contract()
        redeem_script = CScript([OP_1])
        outpoint = self.send_spendable_tx([OP_HASH160, hash160(redeem_script), OP_EQUAL])
        tx = self.make_contract_tx_spending_outpoint(outpoint)
        tx.vin[0].scriptSig = CScript([CScript([OP_1])]) # redeem_script
        assert_raises_message(JSONRPCException, "bad-txns-invalid-sender-script", self.node.sendrawtransaction, bytes_to_hex_str(tx.serialize()))
        self.node.generate(1)

        # Do the same test but make sure that a block containing such a tx is rejected
        block = self.create_block_with_txes([tx])
        old_block_count = self.node.getblockcount()
        self.node.submitblock(bytes_to_hex_str(block.serialize()))
        assert_equal(old_block_count, self.node.getblockcount())

    def verify_p2pk_sender_test(self):
        self.setup_address_contract()
        unspent = self.unspents.pop(0)
        tx = self.make_contract_tx_spending_outpoint(COutPoint(int(unspent['txid'], 16), unspent['vout']))
        tx = rpc_sign_transaction(self.node, tx)
        self.node.sendrawtransaction(bytes_to_hex_str(tx.serialize()))
        blockhash = self.node.generate(1)[0]
        block = self.node.getblock(blockhash)
        assert_equal(len(block['tx']), 3) # Since we do a value transfer we will have a condensing tx as well.
        self.assert_sender(p2pkh_to_hex_hash(unspent['address']), p2pkh_to_hex_hash(unspent['address']))

    def verify_p2pkh_sender_test(self):
        self.setup_address_contract()
        address = self.node.getnewaddress()
        outpoint = self.send_spendable_tx([OP_DUP, OP_HASH160, hex_str_to_bytes(p2pkh_to_hex_hash(address)), OP_EQUALVERIFY, OP_CHECKSIG])
        tx = self.make_contract_tx_spending_outpoint(outpoint)
        tx = rpc_sign_transaction(self.node, tx)
        self.node.sendrawtransaction(bytes_to_hex_str(tx.serialize()))
        blockhash = self.node.generate(1)[0]
        block = self.node.getblock(blockhash)
        assert_equal(len(block['tx']), 4) # Since we do a value transfer we will have a condensing tx as well.
        self.assert_sender(p2pkh_to_hex_hash(address), p2pkh_to_hex_hash(address))

    def verify_inter_contract_sender_test(self):
        self.setup_address_contract()
        self.setup_proxy_contract()
        address = self.node.getnewaddress()
        self.node.sendtoaddress(address, 1)
        # call proxy(address)
        self.node.sendtocontract(self.proxy_contract_address, "06713c3e" + (self.contract_address.zfill(64)), 0, 1000000, 0.000001, address)
        self.node.generate(1)
        self.assert_sender(self.proxy_contract_address, p2pkh_to_hex_hash(address))

    def run_test(self):
        self.node.generate(1000)
        self.unspents = self.node.listunspent()
        self.verify_non_standard_sender_test()
        self.verify_p2sh_sender_test()
        self.verify_p2pk_sender_test()
        self.verify_p2pkh_sender_test()
        self.verify_inter_contract_sender_test()

if __name__ == '__main__':
    QtumSenderTest().main()