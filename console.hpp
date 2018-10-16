#include <cstdint>

#pragma pack(push, 1)
struct alignas(1) VGACell
{
    char character;
    union { struct {
        int foreground_color : 4;
        int unknown_flag : 1;
        int background_color : 3;
    };
    uint8_t pen;
    };
};
static_assert(sizeof(VGACell) == 2);
#pragma pack(pop)

class Console
{
    static int constexpr screen_width = 80;
    static int constexpr screen_height = 25;
    static int constexpr buffer_size = screen_width * screen_height;
    static inline VGACell * vram_base_address() {
        return reinterpret_cast<VGACell * const>(0xb8000ull);
    }
    uint16_t cursorPosition;
    uint8_t pen;
    void updateCursor();
    void outChar(char c);
    void lineFeed();
    void carriageReturn();
    uint16_t setCursor(uint16_t newValue) {
         return cursorPosition = (newValue % buffer_size);
    };
    //print format string until an arg is met (or format ends), print the arg (if matching)
    const char* _printf(const char* format, char const * arg);
    const char* _printf(const char* format, int64_t arg);
public:
    void initialize();
    void clearScreen();
    VGACell& cellAt(uint16_t pos) { return (vram_base_address()[pos % buffer_size]); }
    void enableCursor();
    void disableCursor();
    uint16_t coordToPos(int x, int y);
    void moveCursor(uint16_t pos);
    void moveCursor(int x, int y);

    void writeString(char const * c );
    void writeNumber(int64_t number, int minWidth=1, int base = 10);

    template<typename firstArg, typename ... argTypes>
    void printf(const char* format, firstArg arg, argTypes ... args ) {
        auto argument = _printf(format,arg);
        printf(argument, args ...);
    }

    void printf(const char* format) {
        while (*format) {
            format = _printf(format, "%s");
        }
        updateCursor();
    }
};

extern Console console;