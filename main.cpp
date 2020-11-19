/*
 * Copyright (c) 2020, CATIE
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mbed.h"

using namespace sixtron;

namespace {
#define PERIOD_MS 1000
}

static DigitalOut led1(LED1);

// main() runs in its own thread in the OS
// (note the calls to ThisThread::sleep_for below for delays)
int main()
{
    while (true) {
        led1 = !led1;
        if (led1) {
            printf("Alive!\n");
        }
        ThisThread::sleep_for(PERIOD_MS / 2);
    }
}
