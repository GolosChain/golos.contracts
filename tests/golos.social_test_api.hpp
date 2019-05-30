#pragma once
#include "test_api_helper.hpp"
#include "../golos.social/types.h"

namespace eosio { namespace testing {


struct golos_social_api: base_contract_api {
    golos_social_api(golos_tester* tester, name code, symbol sym)
    :   base_contract_api(tester, code)
    ,   _symbol(sym) {}
    symbol _symbol;

    //// social actions
    action_result pin(account_name pinner, account_name pinning) {
        return push("pin"_n, pinner, args()
            ("pinner", pinner)
            ("pinning", pinning)
        );
    }

    action_result unpin(account_name pinner, account_name pinning) {
        return push("unpin"_n, pinner, args()
            ("pinner", pinner)
            ("pinning", pinning)
        );
    }

    action_result block(account_name blocker, account_name blocking) {
        return push("block"_n, blocker, args()
            ("blocker", blocker)
            ("blocking", blocking)
        );
    }

    action_result unblock(account_name blocker, account_name blocking) {
        return push("unblock"_n, blocker, args()
            ("blocker", blocker)
            ("blocking", blocking)
        );
    }

    action_result updatemeta(account_name account, accountmeta meta) {
        return push("updatemeta"_n, account, args()
            ("account", account)
            ("meta", meta)
        );
    }

    action_result deletemeta(account_name account) {
        return push("deletemeta"_n, account, args()
            ("account", account)
        );
    }
    
    //// social tables
    std::vector<variant> get_pinblocks(account_name pinner_blocker) {
        return _tester->get_all_chaindb_rows(_code, pinner_blocker, N(pinblock), false);
    }

    variant get_accountmeta(account_name acc) {
        return _tester->get_chaindb_singleton(_code, acc, "accountmeta"_n, "get accountmeta");
    }
};


}} // eosio::testing
