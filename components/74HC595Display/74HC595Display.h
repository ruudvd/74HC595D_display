#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/defines.h"
#include "esphome/components/display/display_buffer.h"

#ifdef USE_TIME
#include "esphome/components/time/real_time_clock.h"
#endif

namespace esphome
{
    namespace LedDisplay_ns
    {

        enum ScrollMode
        {
            CONTINUOUS = 0,
            STOP,
        };

        class LedDisplayComponent;

        using ledDisplay_writer_t = std::function<void(LedDisplayComponent &)>;

        class LedDisplayComponent : public PollingComponent,
                                    public display::DisplayBuffer
        {
        public:
            void set_writer(ledDisplay_writer_t &&writer) { this->writer_local_ = writer; };

            void setup() override;

            void loop() override;

            void dump_config() override;

            void update() override;

            float get_setup_priority() const override;

            void display();

            void invert_on_off(bool on_off);
            void invert_on_off();

            void turn_on_off(bool on_off);

            void draw_absolute_pixel_internal(int x, int y, Color color) override;
            int get_height_internal() override;
            int get_width_internal() override;

            void set_num_chips(uint8_t num_chips) { this->num_chips_ = num_chips; };
            void set_num_chip_lines(uint8_t num_chip_lines) { this->num_chip_lines_ = num_chip_lines; };

            void set_scroll_speed(uint16_t speed) { this->scroll_speed_ = speed; };
            void set_scroll_dwell(uint16_t dwell) { this->scroll_dwell_ = dwell; };
            void set_scroll_delay(uint16_t delay) { this->scroll_delay_ = delay; };
            void set_scroll(bool on_off) { this->scroll_ = on_off; };
            void set_scroll_mode(ScrollMode mode) { this->scroll_mode_ = mode; };
            void set_reverse(bool on_off) { this->reverse_ = on_off; };

            void send_char(uint8_t chip, uint8_t data);
            void send64pixels(uint8_t chip, const uint8_t pixels[8]);

            void scroll_left();
            void scroll(bool on_off, ScrollMode mode, uint16_t speed, uint16_t delay, uint16_t dwell);
            void scroll(bool on_off, ScrollMode mode);
            void scroll(bool on_off);
            void intensity(uint8_t intensity);

            /// Evaluate the printf-format and print the result at the given position.
            uint8_t printdigitf(uint8_t pos, const char *format, ...) __attribute__((format(printf, 3, 4)));
            /// Evaluate the printf-format and print the result at position 0.
            uint8_t printdigitf(const char *format, ...) __attribute__((format(printf, 2, 3)));

            /// Print `str` at the given position.
            uint8_t printdigit(uint8_t pos, const char *str);
            /// Print `str` at position 0.
            uint8_t printdigit(const char *str);

#ifdef USE_TIME
            /// Evaluate the strftime-format and print the result at the given position.
            uint8_t strftimedigit(uint8_t pos, const char *format, time::ESPTime time) __attribute__((format(strftime, 3, 0)));

            /// Evaluate the strftime-format and print the result at position 0.
            uint8_t strftimedigit(const char *format, time::ESPTime time) __attribute__((format(strftime, 2, 0)));
#endif

        protected:
            void send_byte_(uint8_t a_register, uint8_t data);
            //void send_to_all_(uint8_t a_register, uint8_t data);

            uint8_t num_chips_;
            uint8_t num_chip_lines_;

            bool scroll_;
            bool reverse_;
            bool update_{false};

            uint16_t scroll_speed_;
            uint16_t scroll_delay_;
            uint16_t scroll_dwell_;
            uint16_t old_buffer_size_ = 0;
            ScrollMode scroll_mode_;
            bool invert_ = false;
            uint8_t bckgrnd_ = 0x0;
            std::vector<std::vector<bool>> led_displaybuffer_;
            uint32_t last_scroll_ = 0;
            uint16_t stepsleft_;
            size_t get_buffer_length_();
            optional<ledDisplay_writer_t> writer_local_{};

            // gpio
            static const gpio_num_t ROW1 = GPIO_NUM_12;
            static const gpio_num_t ROW2 = GPIO_NUM_14;
            static const gpio_num_t ROW3 = GPIO_NUM_27;
            static const gpio_num_t ROW4 = GPIO_NUM_26;
            static const gpio_num_t ROW5 = GPIO_NUM_25;
            static const gpio_num_t ROW6 = GPIO_NUM_33;
            static const gpio_num_t ROW7 = GPIO_NUM_32;

            static const gpio_num_t ShiftClock = GPIO_NUM_5;
            static const gpio_num_t ShiftData = GPIO_NUM_16;
            static const gpio_num_t ShiftClear = GPIO_NUM_17;
            static const gpio_num_t LatchClock = GPIO_NUM_17;
            static const gpio_num_t MasterClr = GPIO_NUM_18;

            std::vector<gpio_num_t> rows = {ROW7, ROW6, ROW5, ROW4, ROW3, ROW2, ROW1};

            //  static const int MAX_COLUMNS = 80;
            //  static const int MAX_ROWS = 7;
            //  static const int COL_CHAR = 6;
        };

    } // namespace LedDisplay_ns
} // namespace esphome
