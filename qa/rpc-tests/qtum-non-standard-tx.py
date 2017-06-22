#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.qtum import make_transaction, make_vin
import sys


class QtumNonStandardTxTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-txindex=1']]*2)
        self.node = self.nodes[0]
        self.is_network_split = False
        connect_nodes(self.nodes[0], 1)

    def check_tx_script(self, scriptPubKeys, rejected=True):
        vout = []
        for scriptPubKey in scriptPubKeys:
            vout.append(CTxOut(0, scriptPubKey))

        tx_hex = make_transaction(self.node, [make_vin(self.node, int(1*COIN))], vout)
        if rejected:
            assert_raises(JSONRPCException, self.node.sendrawtransaction, tx_hex)
        else:
            self.node.sendrawtransaction(tx_hex)

    def too_many_params_test(self):
        correct_call_ops = [1, 100000, 1, b"\x00", hex_str_to_bytes(self.contract_address)]
        correct_create_ops = [1, 100000, 1, b"\x00"]

        # Too many params (insert at back)
        for i in range(1, 10):
            for correct_ops, final_op in [(correct_call_ops, OP_CALL), (correct_create_ops, OP_CREATE)]:
                test_ops = correct_ops + [b"\x01"]*i + [final_op]
                self.check_tx_script([CScript(test_ops)])

        # Too many params (insert at front)
        for i in range(1, 10):
            for correct_ops, final_op in [(correct_call_ops, OP_CALL), (correct_create_ops, OP_CREATE)]:
                test_ops = [b"\x01"]*i + correct_ops + [final_op]
                self.check_tx_script([CScript(test_ops)])

        # Make sure that correct standard txs are accepted in the mempool
        for correct_ops, final_op in [(correct_call_ops, OP_CALL), (correct_create_ops, OP_CREATE)]:
            test_ops = correct_ops + [final_op]
            self.check_tx_script([CScript(test_ops)], rejected=False)

    def too_few_params_test(self):
        correct_call_ops = [1, 100000, 1, b"\x00", hex_str_to_bytes(self.contract_address)]
        correct_create_ops = [1, 100000, 1, b"\x00"]

        for i in range(1, len(correct_call_ops)-1):
            # Too few params for OP_CALL (delete from front)
            test_ops = correct_call_ops[i:] + [OP_CALL]
            self.check_tx_script([CScript(test_ops)])

            # Too few params for OP_CALL (delete from back)
            test_ops = correct_call_ops[0:-1*i] + [OP_CALL]
            self.check_tx_script([CScript(test_ops)])

        for i in range(1, len(correct_create_ops)-1):
            # Too few params for OP_CREATE (delete from front)
            test_ops = correct_create_ops[i:] + [OP_CREATE]
            self.check_tx_script([CScript(test_ops)])
            
            # Too few params for OP_CREATE (delete from back)
            test_ops = correct_create_ops[0:-1*i] + [OP_CREATE]
            self.check_tx_script([CScript(test_ops)])

    def randomize_script_test(self):
        random.seed(1)
        ops = [1, 100000, 1, b"\x00", hex_str_to_bytes(self.contract_address), OP_CALL, OP_CREATE, OP_SPEND]

        for i in range(1000):
            n = random.randint(2, len(ops)-1)
            test_ops = []
            for j in range(n):
                test_ops.append(random.choice(ops))

            # Only select scriptPubKeys so that there exists at least an OP_CALL and OP_CREATE/OP_CALL or two OP_CREATE/OP_CALL (these should always fail)
            if (test_ops.count(OP_SPEND) > 0 and test_ops.count(OP_CREATE)+test_ops.count(OP_CALL) > 0) or test_ops.count(OP_CREATE)+test_ops.count(OP_CALL) > 1:
                self.check_tx_script([CScript(test_ops)])

    def single_op_test(self):
        self.check_tx_script([CScript([OP_CALL])])
        self.check_tx_script([CScript([OP_CREATE])])

    def one_invalid_one_valid_test(self):
        valid_scriptPubKeys = [
            CScript([1, 100000, 1, b"\x00", hex_str_to_bytes(self.contract_address), OP_CALL]),
            CScript([1, 100000, 1, b"\x00", OP_CREATE])
        ]

        invalid_scriptPubKeys = [
            CScript([OP_CALL]),
            CScript([OP_CREATE])
        ]
        for valid_scriptPubKey in valid_scriptPubKeys:
            for invalid_scriptPubKey in invalid_scriptPubKeys:
                self.check_tx_script([
                    valid_scriptPubKey,
                    invalid_scriptPubKey
                ])

    def run_test(self):
        self.node.generate(600)
        self.contract_address = self.node.createcontract("00", 1000000, 0.00000001)['address']
        self.node.generate(1)
        self.sync_all()

        print("Checking too many params")
        self.too_many_params_test()
        print("Checking too few params")
        self.too_few_params_test()
        print("Checking random/shuffled scripts")
        self.randomize_script_test()
        print("Checking single OP_CALL/OP_CREATE")
        self.single_op_test()
        print("Checking tx with one invalid and one valid output")
        self.one_invalid_one_valid_test()

if __name__ == '__main__':
    QtumNonStandardTxTest().main()
