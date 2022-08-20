#ifndef HIDEANDSEEK_TYPES_H
#define HIDEANDSEEK_TYPES_H

#include "hide_and_seek.pb.h"

const double INF = std::numeric_limits<double>::max();

namespace Types {
    using GameView = ir::sharif::aic::hideandseek::api::grpc::GameView;
}

namespace HAS {
    using namespace ir::sharif::aic::hideandseek::api::grpc;
}

#endif
