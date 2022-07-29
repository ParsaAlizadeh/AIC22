#ifndef HIDEANDSEEK_TYPES_H
#define HIDEANDSEEK_TYPES_H

#include "hide_and_seek.pb.h"

namespace Types {
    using GameView = ir::sharif::aic::hideandseek::api::grpc::GameView;
    using Node = ir::sharif::aic::hideandseek::api::grpc::Node;
    using Path = ir::sharif::aic::hideandseek::api::grpc::Path;
    using Graph = ir::sharif::aic::hideandseek::api::grpc::Graph;
}

#endif
