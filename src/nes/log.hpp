//  Program:      nes-py
//  File:         log.hpp
//  Description:  Logging utilities for the project
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_LOG_HPP
#define NES_LOG_HPP

#include <iostream>

namespace NES {

/// a NOP debugging statement
#define DEBUG(x) do {} while (0)
/// the actual debugging statement
// #define DEBUG(x) do { std::cerr << x << std::endl; } while (0)

}  // namespace NES

#endif  // NES_LOG_HPP
