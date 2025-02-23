#pragma once

#include <actor-zeta.hpp>
#include <components/pipeline/context.hpp>
#include <services/collection/operators/operator.hpp>
#include "session_type.hpp"

namespace services::collection::sessions {

    struct suspend_plan_t : public session_base_t<suspend_plan_t> {
        actor_zeta::address_t client;
        operators::operator_ptr plan;
        components::pipeline::context_t pipeline_context;

        suspend_plan_t(actor_zeta::address_t client, operators::operator_ptr&& plan, components::pipeline::context_t&& pipeline_context)
            : client(std::move(client))
            , plan(std::move(plan))
            , pipeline_context(std::move(pipeline_context))
        {}

        static type_t type_impl() {
            return type_t::suspend_plan;
        }
    };

} // namespace services::collection::sessions