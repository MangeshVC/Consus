// Copyright (c) 2015-2016, Robert Escriva, Cornell University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Consus nor the names of its contributors may be
//       used to endorse or promote products derived from this software without
//       specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef consus_txman_daemon_h_
#define consus_txman_daemon_h_

// STL
#include <algorithm>
#include <string>

// po6
#include <po6/net/location.h>
#include <po6/threads/thread.h>

// e
#include <e/compat.h>
#include <e/garbage_collector.h>
#include <e/serialization.h>
#include <e/state_hash_table.h>

// BusyBee
#include <busybee.h>

// Replicant
#include <replicant.h>

// consus
#include "namespace.h"
#include "common/coordinator_link.h"
#include "common/ids.h"
#include "common/network_msgtype.h"
#include "common/transaction_id.h"
#include "common/transaction_group.h"
#include "common/txman.h"
#include "txman/configuration.h"
#include "txman/controller.h"
#include "txman/durable_log.h"
#include "txman/global_voter.h"
#include "txman/kvs_lock_op.h"
#include "txman/kvs_read.h"
#include "txman/kvs_write.h"
#include "txman/local_voter.h"
#include "txman/transaction.h"

BEGIN_CONSUS_NAMESPACE

class daemon
{
    public:
        daemon();
        ~daemon() throw ();

    public:
        int run(bool daemonize,
                std::string data,
                std::string log,
                std::string pidfile,
                bool has_pidfile,
                bool set_bind_to,
                po6::net::location bind_to,
                bool set_coordinator,
                const char* coordinator,
                const char* data_center,
                unsigned threads);

    private:
        struct coordinator_callback;
        struct durable_msg;
        struct durable_cb;
        typedef e::state_hash_table<uint64_t, kvs_read> read_map_t;
        typedef e::state_hash_table<uint64_t, kvs_write> write_map_t;
        typedef e::state_hash_table<uint64_t, kvs_lock_op> lock_op_map_t;
        typedef e::state_hash_table<transaction_group, transaction> transaction_map_t;
        typedef e::state_hash_table<transaction_group, local_voter> local_voter_map_t;
        typedef e::state_hash_table<transaction_group, global_voter> global_voter_map_t;
        typedef e::nwf_hash_map<transaction_group, uint64_t, transaction_group::hash> disposition_map_t;
        typedef std::vector<durable_msg> durable_msg_heap_t;
        typedef std::vector<durable_cb> durable_cb_heap_t;
        friend class controller;
        friend class transaction;
        friend class local_voter;
        friend class global_voter;
        friend class kvs_lock_op;
        friend class kvs_read;
        friend class kvs_write;

    private:
        void loop(size_t thread);
        void process_unsafe_read(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_unsafe_write(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_unsafe_lock_op(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_begin(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_read(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_write(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_commit(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_abort(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_wound(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_paxos_2a(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_paxos_2b(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_lv_vote_1a(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_lv_vote_1b(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_lv_vote_2a(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_lv_vote_2b(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_lv_vote_learn(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_commit_record(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_gv_propose(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_gv_vote_1a(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_gv_vote_1b(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_gv_vote_2a(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_gv_vote_2b(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_kvs_rep_rd_resp(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_kvs_rep_wr_resp(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        void process_kvs_lock_op_resp(comm_id id, std::auto_ptr<e::buffer> msg, e::unpacker up);
        kvs_read* create_read(read_map_t::state_reference* sr);
        kvs_write* create_write(write_map_t::state_reference* sr);
        kvs_lock_op* create_lock_op(lock_op_map_t::state_reference* sr);

    public:
        configuration* get_config();
        void debug_dump();
        uint64_t generate_nonce();
        transaction_id generate_txid();
        uint64_t resend_interval() { return PO6_SECONDS; }
        bool send(comm_id id, std::auto_ptr<e::buffer> msg);
        unsigned send(paxos_group_id g, std::auto_ptr<e::buffer> msg);
        unsigned send(const paxos_group& g, std::auto_ptr<e::buffer> msg);
        // XXX next two might cause excessive logging; investigate
        void send_when_durable(const std::string& entry, comm_id id, std::auto_ptr<e::buffer> msg);
        void send_when_durable(const std::string& entry, const comm_id* ids, e::buffer** msgs, size_t sz);
        void send_when_durable(int64_t idx, paxos_group_id g, std::auto_ptr<e::buffer> msg);
        void send_when_durable(int64_t idx, comm_id id, std::auto_ptr<e::buffer> msg);
        void send_when_durable(int64_t idx, const comm_id* ids, e::buffer** msgs, size_t sz);
        void send_if_durable(int64_t idx, paxos_group_id g, std::auto_ptr<e::buffer> msg);
        void send_if_durable(int64_t idx, comm_id id, std::auto_ptr<e::buffer> msg);
        void send_if_durable(int64_t idx, const comm_id* ids, e::buffer** msgs, size_t sz);
        void callback_when_durable(const std::string& entry, const transaction_group& tg, uint64_t seqno);
        void durable();
        void pump();

    private:
        txman m_us;
        e::garbage_collector m_gc;
        controller m_busybee_controller;
        std::auto_ptr<busybee_server> m_busybee;
        std::auto_ptr<coordinator_callback> m_coord_cb;
        std::auto_ptr<coordinator_link> m_coord;
        configuration* m_config;
        std::vector<e::compat::shared_ptr<po6::threads::thread> > m_threads;
        transaction_map_t m_transactions;
        local_voter_map_t m_local_voters;
        global_voter_map_t m_global_voters;
        disposition_map_t m_dispositions;
        read_map_t m_readers;
        write_map_t m_writers;
        lock_op_map_t m_lock_ops;
        durable_log m_log;

        // awaiting durability
        po6::threads::thread m_durable_thread;
        po6::threads::mutex m_durable_mtx;
        int64_t m_durable_up_to;
        durable_msg_heap_t m_durable_msgs;
        durable_cb_heap_t m_durable_cbs;

        // state machine pumping
        po6::threads::thread m_pumping_thread;

    private:
        daemon(const daemon&);
        daemon& operator = (const daemon&);
};

END_CONSUS_NAMESPACE

#endif // consus_txman_daemon_h_
