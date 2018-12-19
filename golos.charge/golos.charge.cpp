#include "golos.charge.hpp"

namespace golos {
using namespace eosio;
using namespace atmsp::storable;

static constexpr auto max_arg = static_cast<base_t>(std::numeric_limits<fixp_t>::max());

fixp_t charge::consume_charge(name issuer, name user, symbol_code token_code, uint8_t charge_id, int64_t price_arg, int64_t cutoff_arg, int64_t vesting_price) {
    eosio_assert(cutoff_arg < 0 || price_arg <= cutoff_arg, "price > cutoff");
    eosio_assert(price_arg <= max_arg, "price > max_input");
    eosio_assert(cutoff_arg < 0 || cutoff_arg <= max_arg, "cutoff > max_input");
    fixp_t price = to_fixp(price_arg);
    
    auto charge_symbol = symbol(token_code, charge_id);
    balances balances_table(_self, user.value);
    balances::const_iterator itr = balances_table.find(charge_symbol.raw());
    auto new_val = (itr != balances_table.end()) ? calc_value(_self, user, token_code, *itr, price) : price;
    if(cutoff_arg > 0 && new_val > to_fixp(cutoff_arg)) {
        eosio_assert(vesting_price > 0, "not enough power");
        auto user_vesting = golos::vesting::get_account_unlocked_vesting(config::vesting_name, user, token_code);
        eosio_assert(user_vesting.amount >= vesting_price, "insufficient vesting amount");
        INLINE_ACTION_SENDER(golos::vesting, retire) (config::vesting_name,
            {eosio::token::get_issuer(config::token_name, token_code), golos::config::invoice_name},
            {eosio::asset(vesting_price, user_vesting.symbol), user});
        return FP(itr->value);
    }
    if (itr == balances_table.end()) {
        if (new_val > 0)
            balances_table.emplace(issuer, [&]( auto &item ) {
                item.charge_symbol = charge_symbol.raw();
                item.token_code = token_code;
                item.charge_id = charge_id;
                item.last_update = current_time();
                item.value = new_val.data();
            });
        return new_val;
    }
    if (new_val > 0)
        balances_table.modify(*itr, name(), [&]( auto &item ) {
            item.last_update = current_time();
            item.value = new_val.data();
        });
    else
        balances_table.erase(itr);
    return new_val;
}

void charge::use(name user, symbol_code token_code, uint8_t charge_id, int64_t price_arg, int64_t cutoff_arg, int64_t vesting_price) {
    auto issuer = token::get_issuer(config::token_name, token_code);
    require_auth(issuer);
    consume_charge(issuer, user, token_code, charge_id, price_arg, cutoff_arg, vesting_price);
}

void charge::useandstore(name user, symbol_code token_code, uint8_t charge_id, int64_t stamp_id, int64_t price_arg) {
    auto issuer = token::get_issuer(config::token_name, token_code);
    require_auth(issuer);
    auto new_val = consume_charge(issuer, user, token_code, charge_id, price_arg);
    
    storedvals storedvals_table(_self, user.value);
    auto storedvals_index = storedvals_table.get_index<"symbolstamp"_n>();
    auto k = stored::get_key(token_code, charge_id, stamp_id);
    auto itr = storedvals_index.find(k);
    if (itr != storedvals_index.end())
        storedvals_index.modify(itr, issuer, [&]( auto &item ) { item.value = new_val.data(); });
    else
        storedvals_table.emplace(issuer, [&]( auto &item ) {
            item.id = storedvals_table.available_primary_key();
            item.symbol_stamp = k;
            item.value = new_val.data();
        });
}

void charge::removestored(name user, symbol_code token_code, uint8_t charge_id, int64_t stamp_id) {
    require_auth(name(token::get_issuer(config::token_name, token_code)));
    storedvals storedvals_table(_self, user.value);
    auto storedvals_index = storedvals_table.get_index<"symbolstamp"_n>();
    auto itr = storedvals_index.find(stored::get_key(token_code, charge_id, stamp_id));
    eosio_assert(itr != storedvals_index.end(), "itr == storedvals_index.end()");
    storedvals_index.erase(itr);
}

void charge::setrestorer(symbol_code token_code, uint8_t charge_id, std::string func_str, 
        int64_t max_prev, int64_t max_vesting, int64_t max_elapsed) {
    
    eosio_assert(max_prev <= max_arg, "max_prev > max_input");
    eosio_assert(max_vesting <= max_arg, "max_vesting > max_input");
    eosio_assert(max_elapsed <= max_arg, "max_elapsed > max_input");
    auto issuer = token::get_issuer(config::token_name, token_code);
    require_auth(issuer);
    auto charge_symbol = symbol(token_code, charge_id);
    restorers restorers_table(_self, _self.value);
    auto itr = restorers_table.find(charge_symbol.raw());
    
    atmsp::parser<fixp_t> pa;
    atmsp::machine<fixp_t> machine;
    
    bytecode func;
    pa(machine, func_str, "p,v,t");//prev value, vesting, time(elapsed seconds)
    func.from_machine(machine);
    
    if (itr != restorers_table.end())
        restorers_table.modify(*itr, issuer, [&]( auto &item ) {
             item.func = func;
             item.max_prev = to_fixp(max_prev).data();
             item.max_vesting = to_fixp(max_vesting).data();
             item.max_elapsed = to_fixp(max_elapsed).data();
        });
    else
        restorers_table.emplace(issuer, [&]( auto &item ) {
             item.charge_symbol = charge_symbol.raw();
             item.token_code = token_code;
             item.charge_id = charge_id;
             item.func = func;
             item.max_prev = to_fixp(max_prev).data();
             item.max_vesting = to_fixp(max_vesting).data();
             item.max_elapsed = to_fixp(max_elapsed).data();
        });
}
EOSIO_DISPATCH(charge, (use)(useandstore)(removestored)(setrestorer) )

} /// namespace golos

