#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import ComparisonTestFramework
from test_framework.comptool import TestManager, TestInstance, RejectResult
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.blocktools import *
from test_framework.key import CECKey
import io
import struct

class QtumPOSTest(ComparisonTestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 1
        self.tip = None

    def add_options(self, parser):
        super().add_options(parser)
        parser.add_option("--runbarelyexpensive", dest="runbarelyexpensive", default=True)

    def run_test(self):
        self.test = TestManager(self, self.options.tmpdir)
        self.test.add_all_connections(self.nodes)
        NetworkThread().start() # Start up network handling in another thread
        self.test.run()

    def create_unsigned_pos_block(self, staking_prevouts, nTime=None, outNValue=10002, signStakeTx=True):
        if not nTime:
            nTime = ((int(time.time())+200) >> 4) << 4

        parent_block_stake_modifier = int(self.node.getblock(self.node.getbestblockhash())['modifier'], 16)
        parent_block_raw_hex = self.node.getblock(self.node.getbestblockhash(), False)
        f = io.BytesIO(hex_str_to_bytes(parent_block_raw_hex))
        parent_block = CBlock()
        parent_block.deserialize(f)
        coinbase = create_coinbase(self.node.getblockcount()+1)
        coinbase.vout[0].nValue = 0
        coinbase.vout[0].scriptPubKey = b""
        coinbase.rehash()
        block = create_block(int(self.node.getbestblockhash(), 16), coinbase, nTime)
        block.hashPrevBlock = int(self.node.getbestblockhash(), 16)
        if not block.solve_stake(parent_block_stake_modifier, staking_prevouts):
            return None

        # create a new private key used for signing.
        block_sig_key = CECKey()
        block_sig_key.set_secretbytes(hash256(struct.pack('<I', 0xffff)))
        pubkey = block_sig_key.get_pubkey()
        scriptPubKey = CScript([pubkey, OP_CHECKSIG])
        stake_tx_unsigned = CTransaction()
        stake_tx_unsigned.vin.append(CTxIn(block.prevoutStake))
        stake_tx_unsigned.vout.append(CTxOut())
        stake_tx_unsigned.vout.append(CTxOut(int(outNValue*COIN), scriptPubKey))
        stake_tx_unsigned.vout.append(CTxOut(int(outNValue*COIN), scriptPubKey))

        if signStakeTx:
            stake_tx_signed_raw_hex = self.node.signrawtransaction(bytes_to_hex_str(stake_tx_unsigned.serialize()))['hex']
            f = io.BytesIO(hex_str_to_bytes(stake_tx_signed_raw_hex))
            stake_tx_signed = CTransaction()
            stake_tx_signed.deserialize(f)
            block.vtx.append(stake_tx_signed)
        else:
            block.vtx.append(stake_tx_unsigned)
        block.hashMerkleRoot = block.calc_merkle_root()
        return (block, block_sig_key)


    def get_tests(self):
        self.node = self.nodes[0]
        # returns a test case that asserts that the current tip was accepted
        def accepted():
            return TestInstance([[self.tip, True]])

        # returns a test case that asserts that the current tip was rejected
        def rejected(reject = None):
            if reject is None:
                return TestInstance([[self.tip, False]])
            else:
                return TestInstance([[self.tip, reject]])

        # First generate some block so we have some spendable coins
        block_hashes = self.node.generate(40)
        for _ in range(10):
            self.node.sendtoaddress(self.node.getnewaddress(), 1000)
        block_hashes += self.node.generate(1)

        blocks = []
        for block_hash in block_hashes:
            blocks.append(self.node.getblock(block_hash))


        # These are our staking txs
        self.staking_prevouts = []
        self.bad_staking_prevouts = []

        for unspent in self.node.listunspent():
            for block in blocks:
                if unspent['txid'] in block['tx']:
                    tx_block_time = block['time']
                    break
            else:
                assert(False)

            if unspent['confirmations'] > 15:
                self.staking_prevouts.append((COutPoint(int(unspent['txid'], 16), unspent['vout']), int(unspent['amount'])*COIN, tx_block_time))

            if unspent['confirmations'] < 5:
                self.bad_staking_prevouts.append((COutPoint(int(unspent['txid'], 16), unspent['vout']), int(unspent['amount'])*COIN, tx_block_time))


        # First we start off by checking that a valid PoS block is accepted


        # 1 A block that does not have the correct timestamp mask
        t = (int(time.time())+200) | 1
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts, nTime=t)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()

        # 2 A block that with a too high reward
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts, outNValue=30006)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()

        # 3 A block with an incorrect block sig
        bad_key = CECKey()
        bad_key.set_secretbytes(hash256(b'horse staple battery'))
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.sign_block(bad_key)
        self.tip.rehash()
        yield rejected()

        # 4 A block that stakes with txs with too few confirmations
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.bad_staking_prevouts)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()

        # 5 A block that with a coinbase reward
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.vtx[0].vout[0].nValue = 1
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()

        # 6 A block that with no vout in the coinbase
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.vtx[0].vout = []
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()

        # 7 A block way into the future
        t = ((int(time.time())+100000) >> 4) << 4
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts, nTime=t, outNValue=10004)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()

        # 8 No vout in the staking tx
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.vtx[1].vout = []
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()

        # 9 Unsigned stake tx
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts, signStakeTx=False)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()

        # 10 Verify that a valid block is accepted
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts)
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield accepted()
        assert_equal(self.node.getblockcount(), 42)

        # A block without a staking tx.
        t = ((int(time.time())+200) >> 4) << 4
        (self.tip, block_sig_key) = self.create_unsigned_pos_block(self.staking_prevouts, t)
        self.tip.vtx.pop(-1)
        self.tip.hashMerkleRoot = self.tip.calc_merkle_root()
        self.tip.sign_block(block_sig_key)
        self.tip.rehash()
        yield rejected()


if __name__ == '__main__':
    QtumPOSTest().main()
