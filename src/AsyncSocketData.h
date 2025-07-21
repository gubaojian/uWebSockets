/*
 * Authored by Alex Hultman, 2018-2025.
 * Intellectual property of third-party.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UWS_ASYNCSOCKETDATA_H
#define UWS_ASYNCSOCKETDATA_H

#include <string>

namespace uWS {

struct BackPressure {
    std::string buffer;
    unsigned int pendingRemoval = 0;
    bool shrink_to_fit_when_clear = true;

    BackPressure(BackPressure &&other) {
        buffer = std::move(other.buffer);
        pendingRemoval = other.pendingRemoval;
    }
    BackPressure() = default;
    void append(const char *data, size_t length) {
        buffer.append(data, length);
    }
    void erase(unsigned int length) {
        if (length <= 0) {
            return;
        }
        pendingRemoval += length;
        /* Always erase a minimum of 1/2th the current backpressure, then can call drain to fill remain half buffer */
        if (pendingRemoval > (buffer.length() / 2)) {
            size_t remainSize = buffer.length() - pendingRemoval;
            // if buffer is big enough for next read, none need drain, for big remain data memmove is waste time
            if (remainSize > 128*1024) {
                return;
            }
            // for big remain data memmove is waste time, only move data if remain data is not enough for next read.
            if (remainSize != 0) {
                memmove(buffer.data(), buffer.data() + pendingRemoval, remainSize);
                buffer.resize(buffer.length() - pendingRemoval);
            }  else {
                buffer.clear();
            }
            pendingRemoval = 0;
        }
    }
    size_t length() {
        return buffer.length() - pendingRemoval;
    }
    /* Only used in AsyncSocket::write - what about replacing it with the other functions like erase(length())? */
    void clear() {
        pendingRemoval = 0;
        buffer.clear();
        if (shrink_to_fit_when_clear) {
            buffer.shrink_to_fit();
        }
    }
    /* Only used by AsyncSocket::write (optionally) before append */
    void reserve(size_t length) {
        buffer.reserve(length + pendingRemoval);
    }
    /* Only used by getSendBuffer as last resort */
    void resize(size_t length) {
        buffer.resize(length + pendingRemoval);
    }
    const char *data() {
        return buffer.data() + pendingRemoval;
    }
    /* The total length, incuding pending removal */
    size_t totalLength() {
        return buffer.length();
    }

    void setSendBufferNoNeedShinkToFit() {
        shrink_to_fit_when_clear = false;
    }
};

/* Depending on how we want AsyncSocket to function, this will need to change */

template <bool SSL>
struct AsyncSocketData {
    /* This will do for now */
    BackPressure buffer;

    /* Allow move constructing us */
    AsyncSocketData(BackPressure &&backpressure) : buffer(std::move(backpressure)) {

    }

    /* Or emppty */
    AsyncSocketData() = default;
};

}

#endif // UWS_ASYNCSOCKETDATA_H
