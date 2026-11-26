/*
 *     ____  ________     ___                __
 *    / __ \/  _/ __ \   /   |  ____  ____ _/ /_  ______  ___  _____
 *   / /_/ // // / / /  / /| | / __ \/ __ `/ / / / /_  / / _ \/ ___/
 *  / ____// // /_/ /  / ___ |/ / / / /_/ / / /_/ / / /_/  __/ /
 * /_/   /___/_____/  /_/  |_/_/ /_/\__,_/_/\__, / /___/\___/_/
 *                                         /____/
 *
 * M30-EC Gas Detector Firmware
 *
 * File        : ButtonPressDetector.cpp
 * Description : Storage for the button-detector registry.
 * Author      : Jason Wang
 * Version     : see BuildConfig.h, section 4
 *
 * Copyright (c) 2026 PID Analyzer. All rights reserved.
 */
#include "inc/Button.h"

std::vector<ButtonPressDetector*> ButtonPressDetector::s_ButtonPressDetectors;
