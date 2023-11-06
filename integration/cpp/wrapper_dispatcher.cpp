#include "wrapper_dispatcher.hpp"
#include "route.hpp"
#include <core/system_command.hpp>
#include <components/ql/statements/create_collection.hpp>
#include <components/ql/statements/create_database.hpp>
#include <components/ql/statements/drop_collection.hpp>
#include <components/ql/statements/delete_many.hpp>
#include <components/ql/statements/delete_one.hpp>
#include <components/ql/statements/insert_many.hpp>
#include <components/ql/statements/insert_one.hpp>
#include <components/ql/statements/update_many.hpp>
#include <components/ql/statements/update_one.hpp>
#include <components/sql/parser.hpp>

namespace duck_charmer {

    wrapper_dispatcher_t::wrapper_dispatcher_t(actor_zeta::detail::pmr::memory_resource* mr,actor_zeta::address_t manager_dispatcher,log_t &log)
        : actor_zeta::cooperative_supervisor<wrapper_dispatcher_t>(mr,"wrapper_dispatcher")
        , manager_dispatcher_(manager_dispatcher)
        ,log_(log.clone()) {
        add_handler(core::handler_id(core::route::load_finish), &wrapper_dispatcher_t::load_finish);
        add_handler(database::handler_id(database::route::create_database_finish), &wrapper_dispatcher_t::create_database_finish);
        add_handler(database::handler_id(database::route::create_collection_finish), &wrapper_dispatcher_t::create_collection_finish);
        add_handler(database::handler_id(database::route::drop_collection_finish), &wrapper_dispatcher_t::drop_collection_finish);
        add_handler(collection::handler_id(collection::route::insert_finish), &wrapper_dispatcher_t::insert_finish);
        add_handler(collection::handler_id(collection::route::find_finish), &wrapper_dispatcher_t::find_finish);
        add_handler(collection::handler_id(collection::route::find_one_finish), &wrapper_dispatcher_t::find_one_finish);
        add_handler(collection::handler_id(collection::route::delete_finish), &wrapper_dispatcher_t::delete_finish);
        add_handler(collection::handler_id(collection::route::update_finish), &wrapper_dispatcher_t::update_finish);
        add_handler(collection::handler_id(collection::route::size_finish), &wrapper_dispatcher_t::size_finish);
        add_handler(collection::handler_id(collection::route::create_index_finish), &wrapper_dispatcher_t::create_index_finish);
        add_handler(collection::handler_id(collection::route::drop_index_finish), &wrapper_dispatcher_t::drop_index_finish);
    }

    wrapper_dispatcher_t::~wrapper_dispatcher_t() {
        trace(log_, "delete wrapper_dispatcher_t");
    }

    auto wrapper_dispatcher_t::load() -> void {
        session_id_t session;
        trace(log_, "wrapper_dispatcher_t::load session: {}", session.data());
        init();
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            core::handler_id(core::route::load),
            session);
        wait();
    }

    auto wrapper_dispatcher_t::create_database(session_id_t &session, const database_name_t &database) -> void {
        trace(log_, "wrapper_dispatcher_t::create_database session: {}, database name : {} ", session.data(), database);
        init();
        components::ql::create_database_t ql{database};
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            database::handler_id(database::route::create_database),
            session,
            &ql);
        wait();
    }

    auto wrapper_dispatcher_t::create_collection(session_id_t &session, const database_name_t &database, const collection_name_t &collection) -> void {
        trace(log_, "wrapper_dispatcher_t::create_collection session: {}, database name : {} , collection name : {} ", session.data(), database, collection);
        init();
        components::ql::create_collection_t ql{database, collection};
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            database::handler_id(database::route::create_collection),
            session,
            &ql);
        wait();
    }

    auto wrapper_dispatcher_t::drop_collection(components::session::session_id_t &session, const database_name_t &database, const collection_name_t &collection) -> result_drop_collection {
        trace(log_, "wrapper_dispatcher_t::drop_collection session: {}, database name: {}, collection name: {} ", session.data(), database, collection);
        init();
        components::ql::drop_collection_t ql{database, collection};
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            database::handler_id(database::route::drop_collection),
            session,
            &ql);
        wait();
        assert(std::holds_alternative<result_drop_collection>(intermediate_store_) && "[wrapper_dispatcher_t::drop_collection]: return variant intermediate_store_ holds the alternative ");
        return std::get<result_drop_collection>(intermediate_store_);
    }

    auto wrapper_dispatcher_t::insert_one(session_id_t &session, const database_name_t &database, const collection_name_t &collection, document_ptr &document) -> result_insert & {
        trace(log_, "wrapper_dispatcher_t::insert_one session: {}, collection name: {} ", session.data(), collection);
        init();
        components::ql::insert_one_t ql{database, collection, document};
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            collection::handler_id(collection::route::insert_documents),
            session,
            &ql);
        wait();
        assert(std::holds_alternative<result_insert>(intermediate_store_) && "[wrapper_dispatcher_t::insert_one]: return variant intermediate_store_ holds the alternative ");
        return std::get<result_insert>(intermediate_store_);
    }

    auto wrapper_dispatcher_t::insert_many(session_id_t &session, const database_name_t &database, const collection_name_t &collection, std::pmr::vector<document_ptr> &documents) -> result_insert & {
        trace(log_, "wrapper_dispatcher_t::insert_many session: {}, collection name: {} ", session.data(), collection);
        init();
        components::ql::insert_many_t ql{database, collection, documents};
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            collection::handler_id(collection::route::insert_documents),
            session,
            &ql);
        wait();
        assert(std::holds_alternative<result_insert>(intermediate_store_) && "[wrapper_dispatcher_t::insert_many]: return variant intermediate_store_ holds the alternative ");
        return std::get<result_insert>(intermediate_store_);
    }

    auto wrapper_dispatcher_t::find(session_id_t &session, components::ql::aggregate_statement_raw_ptr condition) -> components::cursor::cursor_t* {
        trace(log_, "wrapper_dispatcher_t::find session: {}, database: {} collection: {} ", session.data(), condition->database_, condition->collection_);
        init();
        std::unique_ptr<components::ql::aggregate_statement> ql(condition);
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            collection::handler_id(collection::route::find),
            session,
            ql.get());
        wait();
        assert(std::holds_alternative<components::cursor::cursor_t*>(intermediate_store_) && "[wrapper_dispatcher_t::find]: return variant intermediate_store_ holds the alternative ");
        return std::get<components::cursor::cursor_t*>(intermediate_store_);
    }

    auto wrapper_dispatcher_t::find_one(components::session::session_id_t &session, components::ql::aggregate_statement_raw_ptr condition) -> result_find_one & {
        trace(log_, "wrapper_dispatcher_t::find_one session: {}, database: {} collection: {} ", session.data(), condition->database_, condition->collection_);
        init();
        std::unique_ptr<components::ql::aggregate_statement> ql(condition);
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            collection::handler_id(collection::route::find_one),
            session,
            ql.get());
        wait();
        assert(std::holds_alternative<result_find_one>(intermediate_store_) && "[wrapper_dispatcher_t::find_one]: return variant intermediate_store_ holds the alternative ");
        return std::get<result_find_one>(intermediate_store_);
    }

    auto wrapper_dispatcher_t::delete_one(components::session::session_id_t &session, components::ql::aggregate_statement_raw_ptr condition) -> result_delete & {
        trace(log_, "wrapper_dispatcher_t::delete_one session: {}, database: {} collection: {} ", session.data(), condition->database_, condition->collection_);
        init();
        components::ql::delete_one_t ql{condition};
        std::unique_ptr<components::ql::ql_statement_t> _(condition);
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            collection::handler_id(collection::route::delete_documents),
            session,
            &ql);
        wait();
        assert(std::holds_alternative<result_delete>(intermediate_store_) && "[wrapper_dispatcher_t::delete_one]: return variant intermediate_store_ holds the alternative ");
        return std::get<result_delete>(intermediate_store_);
    }

    auto wrapper_dispatcher_t::delete_many(components::session::session_id_t &session, components::ql::aggregate_statement_raw_ptr condition) -> result_delete &{
        trace(log_, "wrapper_dispatcher_t::delete_many session: {}, database: {} collection: {} ", session.data(), condition->database_, condition->collection_);
        init();
        components::ql::delete_many_t ql{condition};
        std::unique_ptr<components::ql::ql_statement_t> _(condition);
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            collection::handler_id(collection::route::delete_documents),
            session,
            &ql);
        wait();
        assert(std::holds_alternative<result_delete>(intermediate_store_) && "[wrapper_dispatcher_t::delete_many]: return variant intermediate_store_ holds the alternative ");
        return std::get<result_delete>(intermediate_store_);
    }

    auto wrapper_dispatcher_t::update_one(components::session::session_id_t &session, components::ql::aggregate_statement_raw_ptr condition, document_ptr update, bool upsert) -> result_update & {
        trace(log_, "wrapper_dispatcher_t::update_one session: {}, database: {} collection: {} ", session.data(), condition->database_, condition->collection_);
        init();
        components::ql::update_one_t ql{condition, update, upsert};
        std::unique_ptr<components::ql::ql_statement_t> _(condition);
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            collection::handler_id(collection::route::update_documents),
            session,
            &ql);
        wait();
        assert(std::holds_alternative<result_update>(intermediate_store_) && "[wrapper_dispatcher_t::update_one]: return variant intermediate_store_ holds the alternative ");
        return std::get<result_update>(intermediate_store_);
    }

    auto wrapper_dispatcher_t::update_many(components::session::session_id_t &session, components::ql::aggregate_statement_raw_ptr condition, document_ptr update, bool upsert) -> result_update & {
        trace(log_, "wrapper_dispatcher_t::update_many session: {}, database: {} collection: {} ", session.data(), condition->database_, condition->collection_);
        init();
        components::ql::update_many_t ql{condition, update, upsert};
        std::unique_ptr<components::ql::ql_statement_t> _(condition);
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            collection::handler_id(collection::route::update_documents),
            session,
            &ql);
        wait();
        assert(std::holds_alternative<result_update>(intermediate_store_) && "[wrapper_dispatcher_t::update_many]: return variant intermediate_store_ holds the alternative ");
        return std::get<result_update>(intermediate_store_);
    }

    auto wrapper_dispatcher_t::size(session_id_t &session, const database_name_t &database, const collection_name_t &collection) -> result_size {
        trace(log_, "wrapper_dispatcher_t::size session: {}, collection name : {} ", session.data(), collection);
        init();
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            collection::handler_id(collection::route::size),
            session,
            database,
            collection);
        wait();
        assert(std::holds_alternative<result_size>(intermediate_store_) && "[wrapper_dispatcher_t::size]: return variant intermediate_store_ holds the alternative ");
        return std::get<result_size>(intermediate_store_);
    }

    auto wrapper_dispatcher_t::create_index(session_id_t &session, components::ql::create_index_t index) -> result_create_index {
        trace(log_, "wrapper_dispatcher_t::create_index session: {}, index: {}", session.data(), index.name());
        init();
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            collection::handler_id(collection::route::create_index),
            session,
            std::move(index));
        wait();
        assert(std::holds_alternative<result_create_index>(intermediate_store_) && "[wrapper_dispatcher_t::create_index]: return variant intermediate_store_ holds the alternative ");
        return std::get<result_create_index>(intermediate_store_);
    }

    auto wrapper_dispatcher_t::drop_index(session_id_t &session, components::ql::drop_index_t drop_index) -> result_drop_index {
        trace(log_, "wrapper_dispatcher_t::drop_index session: {}, index: {}", session.data(), drop_index.name());
        init();
        actor_zeta::send(
            manager_dispatcher_,
            address(),
            collection::handler_id(collection::route::drop_index),
            session,
            std::move(drop_index));
        wait();
        assert(std::holds_alternative<result_drop_index>(intermediate_store_) && "[wrapper_dispatcher_t::drop_index]: return variant intermediate_store_ holds the alternative ");
        return std::get<result_drop_index>(intermediate_store_);
    }

    auto wrapper_dispatcher_t::execute_ql(session_id_t &session, components::ql::variant_statement_t &query) -> result_t {
        using namespace components::ql;

        trace(log_, "wrapper_dispatcher_t::execute session: {}", session.data());

        return std::visit([&](auto& ql) {
            using type = std::decay_t<decltype(ql)>;
            if constexpr (std::is_same_v<type, aggregate_statement>) {
                return send_ql<components::cursor::cursor_t*>(session, ql, "find", collection::handler_id(collection::route::find));
            } else if constexpr (std::is_same_v<type, insert_many_t>) {
                return send_ql<result_insert>(session, ql, "insert", collection::handler_id(collection::route::insert_documents));
            } else if constexpr (std::is_same_v<type, delete_many_t>) {
                return send_ql<result_delete>(session, ql, "delete", collection::handler_id(collection::route::delete_documents));
            } else if constexpr (std::is_same_v<type, update_many_t>) {
                return send_ql<result_update>(session, ql, "update", collection::handler_id(collection::route::update_documents));
            } else {
                return result_t{null_result{}};
            }
        }, query);
    }

    result_t wrapper_dispatcher_t::execute_sql(components::session::session_id_t& session, const std::string& query) {
        trace(log_, "wrapper_dispatcher_t::execute sql session: {}", session.data());
        auto parse_result = components::sql::parse(resource(), query);
        if (parse_result.error) {
            error(log_, parse_result.error.what());
            //todo: output pos error in sql-query
        } else {
            return execute_ql(session, parse_result.ql);
        }
        return null_result{};
    }

    auto wrapper_dispatcher_t::scheduler_impl() noexcept -> actor_zeta::scheduler_abstract_t* {
        assert("wrapper_dispatcher_t::executor_impl");
        return nullptr;
    }

    auto wrapper_dispatcher_t::enqueue_impl(actor_zeta::message_ptr msg, actor_zeta::execution_unit*) -> void {
        std::unique_lock<spin_lock> _(input_mtx_);
        auto tmp = std::move(msg);
        trace(log_, "wrapper_dispatcher_t::enqueue_base msg type: {}", tmp->command().integer_value());
        set_current_message(std::move(tmp));
        execute(this,current_message());
    }

    auto wrapper_dispatcher_t::load_finish() -> void {
        trace(log_, "wrapper_dispatcher_t::load_finish");
        notify();
    }

    auto wrapper_dispatcher_t::create_database_finish(session_id_t &session, services::database::database_create_result result) -> void {
        trace(log_, "wrapper_dispatcher_t::create_database_finish session: {} , result: {} ", session.data(), result.created_);
        intermediate_store_ = result;
        input_session_ = session;
        notify();
    }

    auto wrapper_dispatcher_t::create_collection_finish(session_id_t &session, services::database::collection_create_result result) -> void {
        intermediate_store_ = result;
        input_session_ = session;
        notify();
    }

    void wrapper_dispatcher_t::drop_collection_finish(session_id_t &session, result_drop_collection result) {
        intermediate_store_ = result;
        input_session_ = session;
        notify();
    }

    void wrapper_dispatcher_t::insert_finish(session_id_t &session, result_insert result) {
        trace(log_, "wrapper_dispatcher_t::insert_finish session: {}, result: {} inserted", session.data(), result.inserted_ids().size());
        intermediate_store_ = result;
        input_session_ = session;
        notify();
    }

    auto wrapper_dispatcher_t::find_finish(session_id_t &session, components::cursor::cursor_t* cursor) -> void {
        intermediate_store_ = cursor;
        input_session_ = session;
        notify();
    }

    void wrapper_dispatcher_t::find_one_finish(session_id_t &session, result_find_one result) {
        intermediate_store_ = result;
        input_session_ = session;
        notify();
    }

    void wrapper_dispatcher_t::delete_finish(session_id_t &session, result_delete result) {
        intermediate_store_ = result;
        input_session_ = session;
        notify();
    }

    void wrapper_dispatcher_t::update_finish(session_id_t &session, result_update result) {
        intermediate_store_ = result;
        input_session_ = session;
        notify();
    }

    auto wrapper_dispatcher_t::size_finish(session_id_t &session, result_size result) -> void {
        intermediate_store_ = result;
        input_session_ = session;
        notify();
    }

    auto wrapper_dispatcher_t::create_index_finish(session_id_t &session, result_create_index result) -> void {
        intermediate_store_ = result;
        input_session_ = session;
        notify();
    }

    auto wrapper_dispatcher_t::drop_index_finish(session_id_t &session, result_drop_index result) -> void {
        intermediate_store_ = result;
        input_session_ = session;
        notify();
    }

    void wrapper_dispatcher_t::init() {
        i = 0;
    }

    void wrapper_dispatcher_t::wait() {
        std::unique_lock<std::mutex> lk(output_mtx_);
        cv_.wait(lk, [this]() { return i == 1; });
    }

    void wrapper_dispatcher_t::notify() {
        i = 1;
        cv_.notify_all();
    }

    template <typename Tres, typename Tql>
    auto wrapper_dispatcher_t::send_ql(session_id_t &session, Tql& ql, std::string_view title, uint64_t handle) -> result_t {
        trace(log_, "wrapper_dispatcher_t::{} session: {}, database: {} collection: {} ",
              title, session.data(), ql.database_, ql.collection_);
        init();
        actor_zeta::send(
                    manager_dispatcher_,
                    address(),
                    handle,
                    session,
                    &ql);
        wait();
        assert(std::holds_alternative<Tres>(intermediate_store_) && "[wrapper_dispatcher_t::send_ql]: return variant intermediate_store_ holds the alternative ");
        return std::get<Tres>(intermediate_store_);
    }

} // namespace python
