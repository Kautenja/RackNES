//  Program:      nes-py
//  File:         log.hpp
//  Description:  Logging utilities for the project
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_LOG_HPP
#define NES_LOG_HPP

#include <iostream>

#define debug_disabled true

#define LOG \
    if (debug_disabled) {} \
    else std::cerr

#endif  // NES_LOG_HPP
