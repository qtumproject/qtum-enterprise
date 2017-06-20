#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import *
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.address import *
from test_framework.qtum import *
import io

class QtumScriptPubKeyTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-staking=0']])
        self.is_network_split = False
        self.node = self.nodes[0]

    def run_test(self):
        self.node.generate(20)
        ops = [1, OP_SPEND, OP_CREATE]
        scriptPubKey = CScript(ops)
        tx = CTransaction()
        tx.vin = [make_vin(self.node, 20000*COIN)]
        tx.vout = [CTxOut(int(19999.9*COIN), scriptPubKey)]
        tx.rehash()
        sign_res = self.node.signrawtransaction(bytes_to_hex_str(tx.serialize()))
        tx_hex = sign_res['hex']
        f = io.BytesIO(hex_str_to_bytes(tx_hex))
        tx = CTransaction()
        tx.deserialize(f)
        try:
            self.node.sendrawtransaction(bytes_to_hex_str(tx.serialize()))
        except:
            pass

        # This line is only to check whether or not qtumd has crashed.
        self.node.getblockcount()


if __name__ == '__main__':
    QtumScriptPubKeyTest().main()
