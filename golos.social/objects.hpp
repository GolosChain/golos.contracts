#pragma once

#include <eosiolib/eosio.hpp>

namespace golos {

using namespace eosio;

namespace structures {

struct pinblock_record {
    pinblock_record() = default;

    account_name account;
    bool pinning = false;
    bool blocking = false;

    auto primary_key() const {
        return account;
    }

    EOSLIB_SERIALIZE(pinblock_record, (account)(pinning)(blocking))
};

} // structures

namespace tables {

using namespace eosio;

using pb_account_idx = indexed_by<N(account), const_mem_fun<structures::pinblock_record, account_name, &structures::pinblock_record::primary_key>>;
using pinblock_table = multi_index<N(pinblock), structures::pinblock_record, pb_account_idx>;
}


} // golos