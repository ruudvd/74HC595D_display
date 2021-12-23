#include "74HC595Display.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

#define SET_LED(value)                                    \
    digitalWrite(ShiftData, m_inverted ? !value : value); \
    delayMicroseconds(1);                                 \
    digitalWrite(ShiftClock, true);                       \
    delayMicroseconds(1);                                 \
    digitalWrite(ShiftClock, false);

namespace esphome
{
    namespace LedDisplay_ns
    {

        static const char *const TAG = "74HC595Display";

        float LedDisplayComponent::get_setup_priority() const { return setup_priority::PROCESSOR; }

        void LedDisplayComponent::setup()
        {
            ESP_LOGCONFIG(TAG, "Setting up 74HC595Display...");

            this->stepsleft_ = 0;
            for (int chip_line = 0; chip_line < this->num_chip_lines_; chip_line++)
            {
                std::vector<bool> vec(1);
                this->led_displaybuffer_.push_back(vec);
                // Initialize buffer with 0 for display so all non written pixels are blank
                this->led_displaybuffer_[chip_line].resize(get_width_internal(), 0);
            }
            /*
            // let's assume the user has all 8 digits connected, only important in daisy chained setups anyway
            this->send_to_all_(MAX7219_REGISTER_SCAN_LIMIT, 7);
            // let's use our own ASCII -> led pattern encoding
            this->send_to_all_(MAX7219_REGISTER_DECODE_MODE, 0);
            // No display test with all the pixels on
            this->send_to_all_(MAX7219_REGISTER_DISPLAY_TEST, MAX7219_NO_DISPLAY_TEST);
            // SET Intsity of display
            this->send_to_all_(MAX7219_REGISTER_INTENSITY, this->intensity_);
            // this->send_to_all_(MAX7219_REGISTER_INTENSITY, 1);
            this->display();
            // power up
            this->send_to_all_(MAX7219_REGISTER_SHUTDOWN, 1);
            */

            // set IO to output
            for (auto row : rows)
            {
                pinMode(row, OUTPUT);
            }
            pinMode(ShiftClock, OUTPUT);
            pinMode(ShiftData, OUTPUT);
            pinMode(ShiftClear, OUTPUT);
            pinMode(MasterClr, OUTPUT);

            // clear and reset display
            digitalWrite(ShiftClear, 0); //default level LOW
            digitalWrite(MasterClr, 0);
            delayMicroseconds(10);
            digitalWrite(MasterClr, 1); //reset display

            if (m_flipped)
            {
                std::reverse(rows.begin(), rows.end());
            }
        }

        void LedDisplayComponent::dump_config()
        {
            ESP_LOGCONFIG(TAG, "74HC595Display:");

            ESP_LOGCONFIG(TAG, "  Scroll Mode: %u", this->scroll_mode_);
            ESP_LOGCONFIG(TAG, "  Scroll Speed: %u", this->scroll_speed_);
            ESP_LOGCONFIG(TAG, "  Scroll Dwell: %u", this->scroll_dwell_);
            ESP_LOGCONFIG(TAG, "  Scroll Delay: %u", this->scroll_delay_);

            LOG_UPDATE_INTERVAL(this);
        }

        void LedDisplayComponent::loop()
        {
            uint32_t now = millis();

            // check if the buffer has shrunk past the current position since last update
            if ((this->led_displaybuffer_[0].size() >= this->old_buffer_size_ + 3) ||
                (this->led_displaybuffer_[0].size() <= this->old_buffer_size_ - 3))
            {
                this->stepsleft_ = 0;
                this->display();
                this->old_buffer_size_ = this->led_displaybuffer_[0].size();
            }

            // Reset the counter back to 0 when full string has been displayed.
            if (this->stepsleft_ > this->led_displaybuffer_[0].size())
                this->stepsleft_ = 0;

            // Return if there is no need to scroll or scroll is off
            if (!this->scroll_ || (this->led_displaybuffer_[0].size() <= (size_t)get_width_internal()))
            {
                this->display();
                return;
            }

            if ((this->stepsleft_ == 0) && (now - this->last_scroll_ < this->scroll_delay_))
            {
                this->display();
                return;
            }

            // Dwell time at end of string in case of stop at end
            if (this->scroll_mode_ == ScrollMode::STOP)
            {
                if (this->stepsleft_ >= this->led_displaybuffer_[0].size() - (size_t)get_width_internal() + 1)
                {
                    if (now - this->last_scroll_ >= this->scroll_dwell_)
                    {
                        this->stepsleft_ = 0;
                        this->last_scroll_ = now;
                        this->display();
                    }
                    return;
                }
            }

            // Actual call to scroll left action
            if (now - this->last_scroll_ >= this->scroll_speed_)
            {
                this->last_scroll_ = now;
                this->scroll_left();
                this->display();
            }

            void LedDisplayComponent::display()
            {
                GPIOPin row;
                for (uint8_t line = 0; line < this->get_height_internal(); line++)
                {
                    for (uint16_t j = 0; j < this->get_width_internal(); j++)
                    {
                        /*
                    if (this->reverse_)
                    {
                    }
                    else
                    {
                    }
                    */
                        SET_LED(this->led_displaybuffer_[chip_line][j]);
                    }
                    row = rows[line];
                    digitalWrite(LatchClock, true);
                    delayMicroseconds(10);
                    digitalWrite(LatchClock, false);
                    digitalWrite(row, true);
                    delayMicroseconds(2000); //was 1000
                    digitalWrite(row, false);
                }
            }

            int LedDisplayComponent::get_height_internal()
            {
                return this->num_chip_lines_;
            }

            int LedDisplayComponent::get_width_internal()
            {
                return this->num_chips_ * 8;
            }

            void HOT LedDisplayComponent::draw_absolute_pixel_internal(int x, int y, Color color)
            {
                if (x + 1 > (int)this->led_displaybuffer_[0].size())
                { // Extend the display buffer in case required
                    for (int chip_line = 0; chip_line < this->num_chip_lines_; chip_line++)
                    {
                        this->led_displaybuffer_[chip_line].resize(x + 1, this->bckgrnd_);
                    }
                }

                if ((y >= this->get_height_internal()) || (y < 0) || (x < 0)) // If pixel is outside display then dont draw
                    return;

                uint16_t pos = x;   // X is starting at 0 top left
                uint8_t subpos = y; // Y is starting at 0 top left

                if (color.is_on())
                {
                    this->led_displaybuffer_[subpos / 8][pos] |= (1 << subpos % 8);
                }
                else
                {
                    this->led_displaybuffer_[subpos / 8][pos] &= ~(1 << subpos % 8);
                }
            }

            void LedDisplayComponent::send_byte_(uint8_t a_register, uint8_t data)
            {
                this->write_byte(a_register); // Write register value to MAX
                this->write_byte(data);       // Followed by actual data
            }
            /*
        void LedDisplayComponent::send_to_all_(uint8_t a_register, uint8_t data)
        {
            // this->enable();                                // Enable SPI
            for (uint8_t i = 0; i < this->num_chips_; i++) // Run the loop for every MAX chip in the stack
                this->send_byte_(a_register, data);        // Send the data to the chips
                                                           // this->disable();                               // Disable SPI
        }
        */
            void LedDisplayComponent::update()
            {
                this->update_ = true;
                for (int chip_line = 0; chip_line < this->num_chip_lines_; chip_line++)
                {
                    this->led_displaybuffer_[chip_line].clear();
                    this->led_displaybuffer_[chip_line].resize(get_width_internal(), this->bckgrnd_);
                }
                if (this->writer_local_.has_value()) // insert Labda function if available
                    (*this->writer_local_)(*this);
            }

            void LedDisplayComponent::invert_on_off(bool on_off) { this->invert_ = on_off; };
            void LedDisplayComponent::invert_on_off() { this->invert_ = !this->invert_; };

            void LedDisplayComponent::turn_on_off(bool on_off)
            {
                /*
            if (on_off)
            {
                this->send_to_all_(MAX7219_REGISTER_SHUTDOWN, 1);
            }
            else
            {
                this->send_to_all_(MAX7219_REGISTER_SHUTDOWN, 0);
            }
            */
            }

            void LedDisplayComponent::scroll(bool on_off, ScrollMode mode, uint16_t speed, uint16_t delay, uint16_t dwell)
            {
                this->set_scroll(on_off);
                this->set_scroll_mode(mode);
                this->set_scroll_speed(speed);
                this->set_scroll_dwell(dwell);
                this->set_scroll_delay(delay);
            }

            void LedDisplayComponent::scroll(bool on_off, ScrollMode mode)
            {
                this->set_scroll(on_off);
                this->set_scroll_mode(mode);
            }

            void LedDisplayComponent::scroll(bool on_off) { this->set_scroll(on_off); }

            void LedDisplayComponent::scroll_left()
            {
                for (int chip_line = 0; chip_line < this->num_chip_lines_; chip_line++)
                {
                    if (this->update_)
                    {
                        this->led_displaybuffer_[chip_line].push_back(this->bckgrnd_);
                        for (uint16_t i = 0; i < this->stepsleft_; i++)
                        {
                            this->led_displaybuffer_[chip_line].push_back(this->led_displaybuffer_[chip_line].front());
                            this->led_displaybuffer_[chip_line].erase(this->led_displaybuffer_[chip_line].begin());
                        }
                    }
                    else
                    {
                        this->led_displaybuffer_[chip_line].push_back(this->led_displaybuffer_[chip_line].front());
                        this->led_displaybuffer_[chip_line].erase(this->led_displaybuffer_[chip_line].begin());
                    }
                }
                this->update_ = false;
                this->stepsleft_++;
            }

            void LedDisplayComponent::send_char(uint8_t chip, uint8_t data)
            {
                // get this character from PROGMEM
                for (uint8_t i = 0; i < 8; i++)
                    this->led_displaybuffer_[0][chip * 8 + i] = progmem_read_byte(&MAX7219_DOT_MATRIX_FONT[data][i]);
            } // end of send_char

            // send one character (data) to position (chip)

            void LedDisplayComponent::send64pixels(uint8_t chip, const uint8_t pixels[8])
            {
                for (uint8_t col = 0; col < 8; col++)
                {                                      // RUN THIS LOOP 8 times until column is 7
                                                       // this->enable();                    // start sending by enabling SPI
                    for (uint8_t i = 0; i < chip; i++) // send extra NOPs to push the pixels out to extra displays
                        this->send_byte_(MAX7219_REGISTER_NOOP,
                                         MAX7219_REGISTER_NOOP); // run this loop unit the matching chip is reached
                    uint8_t b = 0;                               // rotate pixels 90 degrees -- set byte to 0
                    if (this->orientation_ == 0)
                    {
                        for (uint8_t i = 0; i < 8; i++)
                        {
                            // run this loop 8 times for all the pixels[8] received
                            b |= ((pixels[i] >> col) & 1) << (7 - i); // change the column bits into row bits
                        }
                    }
                    else if (this->orientation_ == 1)
                    {
                        b = pixels[col];
                    }
                    else if (this->orientation_ == 2)
                    {
                        for (uint8_t i = 0; i < 8; i++)
                        {
                            b |= ((pixels[i] >> (7 - col)) & 1) << i;
                        }
                    }
                    else
                    {
                        b = pixels[7 - col];
                    }
                    // send this byte to display at selected chip
                    if (this->invert_)
                    {
                        this->send_byte_(col + 1, ~b);
                    }
                    else
                    {
                        this->send_byte_(col + 1, b);
                    }
                    for (int i = 0; i < this->num_chips_ - chip - 1; i++) // end with enough NOPs so later chips don't update
                        this->send_byte_(MAX7219_REGISTER_NOOP, MAX7219_REGISTER_NOOP);
                    // this->disable(); // all done disable SPI
                } // end of for each column
            }     // end of send64pixels

            uint8_t LedDisplayComponent::printdigit(const char *str) { return this->printdigit(0, str); }

            uint8_t LedDisplayComponent::printdigit(uint8_t start_pos, const char *s)
            {
                uint8_t chip = start_pos;
                for (; chip < this->num_chips_ && *s; chip++)
                    send_char(chip, *s++);
                // space out rest
                while (chip < (this->num_chips_))
                    send_char(chip++, ' ');
                return 0;
            } // end of sendString

            uint8_t LedDisplayComponent::printdigitf(uint8_t pos, const char *format, ...)
            {
                va_list arg;
                va_start(arg, format);
                char buffer[64];
                int ret = vsnprintf(buffer, sizeof(buffer), format, arg);
                va_end(arg);
                if (ret > 0)
                    return this->printdigit(pos, buffer);
                return 0;
            }
            uint8_t LedDisplayComponent::printdigitf(const char *format, ...)
            {
                va_list arg;
                va_start(arg, format);
                char buffer[64];
                int ret = vsnprintf(buffer, sizeof(buffer), format, arg);
                va_end(arg);
                if (ret > 0)
                    return this->printdigit(buffer);
                return 0;
            }

#ifdef USE_TIME
            uint8_t LedDisplayComponent::strftimedigit(uint8_t pos, const char *format, time::ESPTime time)
            {
                char buffer[64];
                size_t ret = time.strftime(buffer, sizeof(buffer), format);
                if (ret > 0)
                    return this->printdigit(pos, buffer);
                return 0;
            }
            uint8_t LedDisplayComponent::strftimedigit(const char *format, time::ESPTime time)
            {
                return this->strftimedigit(0, format, time);
            }
#endif

        } // namespace LedDisplay_ns
    }     // namespace esphome
