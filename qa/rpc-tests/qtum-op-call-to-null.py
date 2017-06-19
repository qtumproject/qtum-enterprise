#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import *
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.qtum import *
import sys
import time

class QtumOpCallToNullTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-staking=0']])
        self.is_network_split = False
        self.node = self.nodes[0]

    def op_call_to_null_test(self):
        scriptPubKey = CScript()
        scriptPubKey += b"\x01"                # version
        scriptPubKey += 100000                 # gas_limit
        scriptPubKey += b"\x01"                # gas_price
        scriptPubKey += bytes.fromhex("00")    # data
        scriptPubKey += bytes.fromhex("00"*20) # contract address
        scriptPubKey += OP_CALL

        tx = CTransaction()
        tx.vin = [make_vin(self.node, 2*COIN)]
        tx.vout = [CTxOut(1*COIN, scriptPubKey)]
        tx.rehash()
        
        unsigned_raw_tx = bytes_to_hex_str(tx.serialize_without_witness())
        signed_raw_tx = self.node.signrawtransaction(unsigned_raw_tx)['hex']
        print(self.node.sendrawtransaction(signed_raw_tx))
        self.node.generate(1)
        assert_equal(len(self.node.listcontracts()), 0)

    def run_test(self):
        self.node.generate(20)
        self.op_call_to_null_test()

if __name__ == '__main__':
    QtumOpCallToNullTest().main()
