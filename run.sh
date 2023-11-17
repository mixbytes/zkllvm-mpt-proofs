#!/bin/bash

CLANG_BINARY=${HOME}/zkllvm/build/libs/circifier/llvm/bin/clang

${CLANG_BINARY} \
    -std=c++2a \
    -I/libs/crypto3/block/include \
    -I/libs/crypto3/block/include \
    -I/libs/crypto3/algebra/include \
    -I/libs/crypto3/multiprecision/include \
    -I/libs/crypto3/hash/include \
    src/main.cpp
    # -I${HOME}/zkllvm/libs/stdlib/libcpp \
    # -I${HOME}/zkllvm/libs/crypto3/block/include \
    # -I${HOME}/zkllvm/libs/crypto3/codec/include
    # -I${HOME}/zkllvm/libs/crypto3/containers/include
    #-I${HOME}/zkllvm/libs/crypto3/vdf/include
    # -I${HOME}/zkllvm/libs/crypto3/kdf/include
    # -I${HOME}/zkllvm/libs/crypto3/mac/include
    # -I${HOME}/zkllvm/libs/crypto3/marshalling/core/include
    # -I${HOME}/zkllvm/libs/crypto3/marshalling/algebra/include 
    # -I${HOME}/zkllvm/libs/crypto3/marshalling/zk/include
    # -I${HOME}/zkllvm/libs/crypto3/math/include
    # -I${HOME}/zkllvm/libs/crypto3/modes/include
    # -I${HOME}/zkllvm/libs/crypto3/multiprecision/include
    # -I${HOME}/zkllvm/libs/crypto3/passhash/include
    # -I${HOME}/zkllvm/libs/crypto3/pbkdf/include
    # -I${HOME}/zkllvm/libs/crypto3/pkmodes/include
    # -I${HOME}/zkllvm/libs/crypto3/pkpad/include
    # -I${HOME}/zkllvm/libs/crypto3/pubkey/include
    # -I${HOME}/zkllvm/libs/crypto3/random/include
    # -I${HOME}/zkllvm/libs/crypto3/stream/include
    # -I${HOME}/zkllvm/libs/crypto3/zk/include \
