#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.comptool import TestManager, TestInstance, RejectResult
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.blocktools import *
from test_framework.qtum import *
from test_framework.key import CECKey
import io
import struct

class QtumPOSReorgTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = True

    def remove_from_staking_prevouts(self, staking_prevouts, remove_prevout):
        for j in range(len(staking_prevouts)):
            prevout = staking_prevouts[j]
            if prevout[0].serialize() == remove_prevout.serialize():
                staking_prevouts.pop(j)
                break

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-staking=0'], ['-staking=0']])
        self.is_network_split = False
        connect_nodes_bi(self.nodes, 0, 1)

    def create_and_submit_chain_by_node(self, node, staking_prevouts, fork_tip, num_blocks):
        last_tip = fork_tip
        for i in range(num_blocks):
            nTime = int(fork_tip['time'] + 32*(i+1)) & 0xfffffff0
            for n in self.nodes: n.setmocktime(nTime)
            current_tip, block_sig_key = create_unsigned_pos_block(node, staking_prevouts, nTime=nTime, tip=last_tip)
            current_tip.sign_block(block_sig_key)
            current_tip.rehash()
            node.submitblock(bytes_to_hex_str(current_tip.serialize()))
            self.remove_from_staking_prevouts(staking_prevouts, current_tip.prevoutStake)
            try:
                last_tip = node.getblock(hex(current_tip.sha256)[2:])
            except:
                pass
        self.sync_all()
        return last_tip['hash']

    def run_test(self):
        # First generate some blocks so we have some spendable coins
        # We avoid exceeding 5000 blocks so we don't activate mpos
        self.nodes[0].generate(1500)
        self.sync_all()
        self.nodes[1].generate(2000)
        self.sync_all()
        node_0_staking_prevouts = collect_prevouts(self.nodes[0])
        node_1_staking_prevouts = collect_prevouts(self.nodes[1])
        fork_tip = self.nodes[0].getblock(self.nodes[0].getbestblockhash())

        # Make sure that the nodes always select the chain that it saw first if they are valid and of the same length.
        self.create_and_submit_chain_by_node(self.nodes[0], node_0_staking_prevouts, fork_tip, 1)
        old_best_block_hash = self.nodes[0].getbestblockhash()
        self.create_and_submit_chain_by_node(self.nodes[1], node_1_staking_prevouts, fork_tip, 1)
        assert_equal(old_best_block_hash, self.nodes[0].getbestblockhash())
        assert_equal(old_best_block_hash, self.nodes[1].getbestblockhash())

        # Let node1 orphan the tip
        new_best_block_hash = self.create_and_submit_chain_by_node(self.nodes[1], node_1_staking_prevouts, fork_tip, 2)
        assert(new_best_block_hash != old_best_block_hash)
        assert_equal(new_best_block_hash, self.nodes[0].getbestblockhash())
        assert_equal(new_best_block_hash, self.nodes[1].getbestblockhash())
        old_best_block_hash = new_best_block_hash

        # Orphan the old chain, this time with 500 blocks
        new_best_block_hash = self.create_and_submit_chain_by_node(self.nodes[0], node_0_staking_prevouts, fork_tip, 501)
        assert(new_best_block_hash != old_best_block_hash)
        assert_equal(new_best_block_hash, self.nodes[0].getbestblockhash())
        assert_equal(new_best_block_hash, self.nodes[1].getbestblockhash())
        old_best_block_hash = new_best_block_hash

        # Let node1 attempt to orphan exactly 500 blocks by creating 501 blocks (which should fail)
        new_best_block_hash = self.create_and_submit_chain_by_node(self.nodes[1], node_1_staking_prevouts, fork_tip, 502)
        assert(new_best_block_hash != old_best_block_hash)
        assert_equal(old_best_block_hash, self.nodes[0].getbestblockhash())
        assert_equal(old_best_block_hash, self.nodes[1].getbestblockhash())

        # Change the fork tip to 499 blocks back, which should enable us to orphan the main chain
        fork_tip = self.nodes[1].getblock(self.nodes[1].getblockhash(self.nodes[1].getblockcount()-500))
        old_chain_work = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['chainwork']
        new_best_block_hash = self.create_and_submit_chain_by_node(self.nodes[1], node_1_staking_prevouts, fork_tip, 501)
        assert(new_best_block_hash != old_best_block_hash)
        assert_equal(new_best_block_hash, self.nodes[0].getbestblockhash())
        assert_equal(new_best_block_hash, self.nodes[1].getbestblockhash())
        new_chain_work = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['chainwork']

if __name__ == '__main__':
    QtumPOSReorgTest().main()
