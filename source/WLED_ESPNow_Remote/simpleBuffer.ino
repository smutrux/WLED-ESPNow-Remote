/**********************************************************************
  ESPNow remote for WLED
  Copyright (c) 2024  Damian Schneider (@DedeHai)
  Licensed under the EUPL v. 1.2 or later
  https://github.com/DedeHai/WLED-ESPNow-Remote
***********************************************************************/

#define BUFFER_SIZE 8  // command queue: the longer it is, the longer it takes to send out all commands, 8 gives a good max. delay for rotary encoder

static uint8_t buffer[BUFFER_SIZE];
static size_t head = 0;
static size_t tail = 0;
static size_t count = 0;

// push a value to the back of the buffer
bool IRAM_ATTR buffer_push(uint8_t value) {
    if (count >= BUFFER_SIZE) {
        return false;  // Buffer is full
    }
    buffer[tail] = value;
    tail = (tail + 1) % BUFFER_SIZE;
    count++;
    return true;
}

// read and remove a value from the front of the buffer
bool buffer_read(uint8_t &value) {
    if (count == 0) {
        return false;  // Buffer is empty
    }
    value = buffer[head];
    head = (head + 1) % BUFFER_SIZE;
    count--;
    return true;
}

// peek front value without removing it
bool buffer_peek(uint8_t* value) {
    if (count == 0) {
        return false;  // Buffer is empty
    }
    
    *value = buffer[head];
    return true;
}

// check if buffer is empty
bool buffer_is_empty() {
    return count == 0;
}

// check if buffer is full
bool buffer_is_full() {
    return count >= BUFFER_SIZE;
}

// get current number of elements in the buffer
size_t buffer_get_count() {
    return count;
}

// clear the entire buffer
void buffer_clear() {
    head = 0;
    tail = 0;
    count = 0;
}