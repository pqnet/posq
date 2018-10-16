#include "console.hpp"

Console console;

void Console::enableCursor()
{
    // we cannot use bios because we're in 64 bit mode and he only works in real mode
    // We do port mapped IO in assembly to do the job then :-)
    // cursor size and position is specified by indented line (currently from scanline 14 to scanline 15)
    asm(R"(
        xorq %%rax, %%rax
        xorq %%rdx, %%rdx

        movb $0x0a, %%al
        mov $0x3d4, %%dx
        outb %%al, %%dx

        inb $0x3d5, %%al
        andb $0xc0, %%al
            orb $14, %%al
        mov $0x3d5, %%dx
        outb %%al, %%dx

        movb $0x0b, %%al
        mov $0x3d4, %%dx
        outb %%al, %%dx

        inb $0x3d5, %%al
        andb $0xe0, %%al
            orb $15, %%al
        mov $0x3d5, %%dx
        outb %%al, %%dx

    )"
        :
        :
        : "cc", "%rax", "%rdx");
}

void Console::disableCursor()
{
    // we cannot use bios because we're in 64 bit mode and he only works in real mode
    // We do port mapped IO in assembly to do the job then :-)
    asm(R"(
        xorq %%rax, %%rax
        xorq %%rdx, %%rdx
        movb $0x0a, %%al
        mov $0x3d4, %%dx
         outb %%al, %%dx
        mov $0x20, %%al
        mov $0x3d5, %%dx
         outb %%al, %%dx
    )"
        :
        :
        : "cc", "%rax", "%rdx");
}

uint16_t
Console::coordToPos(int x, int y)
{
    return x + screen_width * y;
}
void Console::moveCursor(int x, int y)
{
    uint16_t pos = coordToPos(x, y);
    moveCursor(pos);
}

void Console::moveCursor(uint16_t pos)
{
    setCursor(pos);
    updateCursor();
}

void Console::updateCursor()
{
    asm(R"(
        xorq %%rax, %%rax
        xorq %%rbx, %%rbx
        xorq %%rdx, %%rdx
        movw %0, %%bx

        movb $0x0e, %%al
        mov $0x3d4, %%dx
        outb %%al, %%dx

        mov %%bh, %%al
        mov $0x3d5, %%dx
        outb %%al, %%dx

        movb $0x0f, %%al
        mov $0x3d4, %%dx
        outb %%al, %%dx

        mov %%bl, %%al
        mov $0x3d5, %%dx
        outb %%al, %%dx

    )"
        :
        : "X"(cursorPosition)
        : "cc", "%rax", "%rbx", "%rdx");
}

void Console::outChar(char c)
{
    auto &cell = cellAt(cursorPosition);
    cell.character = c;
    cell.pen = pen;
    setCursor(cursorPosition + 1);
}

void Console::lineFeed()
{
    setCursor(cursorPosition + screen_width);
}
void Console::carriageReturn()
{
    setCursor(cursorPosition - cursorPosition % screen_width);
}

void Console::writeString(char const *c)
{
    char out;
    while (out = *c++)
    {
        switch (out)
        {
        case '\n':
            carriageReturn();
            lineFeed();
            break;
        default:
            outChar(out);
        }
    }
    updateCursor();
}

void Console::writeNumber(int64_t number, int minWidth, int base)
{
    // allocate a buffer large enough.
    char buffer[65] = {0};
    // remember to write minus for negative numbers
    bool negative = number < 0;
    if (negative)
    {
        number = -number;
    }
    // fill in buffer, from the right
    char *cursor = buffer + 64;
    while (number > 0)
    {
        auto value = number % base;
        number = number / base;
        if (value > 9)
        {
            *(--cursor) = 'a' + value - 10;
        }
        else
        {
            *(--cursor) = '0' + value;
        }
    }
    while (cursor - buffer + minWidth > 64)
    {
        *--cursor = '0';
    }
    writeString(cursor);
}

const char *Console::_printf(const char *format, char const *arg)
{
    while (char out = *format++)
    {
        switch (out)
        {
        case '\n':
            carriageReturn();
            lineFeed();
            break;
        case '%':
            switch (char control = *format++)
            {
            case '%':
                outChar('%');
                break;
            case 's':
                writeString(arg);
                return format;
            default:
                    outChar(out);
                    outChar(control);
                    return format;
            }
            break;
        default:
            outChar(out);
        }
    }
    return format - 1;
}

const char *Console::_printf(const char *format, int64_t arg)
{
    while (char out = *format++)
    {
        switch (out)
        {
        case '\n':
            carriageReturn();
            lineFeed();
            break;
        case '%':
            int minWidth;
            minWidth = 0;
        loop:
                char control;
                control = *format++;
                if (control >= '0' && control <= '9') {
                    minWidth *=10;
                    minWidth += control - '0';
                    goto loop;
                }
                switch (control)
                {
                case '%':
                    outChar('%');
                    break;
                case 'd':
                    writeNumber(arg, minWidth || 1);
                    return format;
                case 'x':
                    writeNumber(arg, minWidth || 1, 16);
                    return format;
                default:
                    outChar(out);
                    outChar(control);
                    return format;
                }
            break;
        default:
            outChar(out);
        }
    }
    return format - 1;
}

void Console::initialize()
{
    cursorPosition = 0;
    pen = 0x7;
}

void Console::clearScreen() {
    auto range_begin = reinterpret_cast<uint64_t*>(vram_base_address());
    auto range_end = range_begin + (buffer_size / 4);
    for (auto i = range_begin; i< range_end; i++) {
        *i = 0;
    }
}