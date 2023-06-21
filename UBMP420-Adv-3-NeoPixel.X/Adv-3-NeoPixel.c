/*==============================================================================
 Project: NeoPixel                      Activity: mirobo.tech/ubmp4-advanced-3
 Date:    June 21, 2023
 
 This advanced programming activity for the mirobo.tech UBMP4 demonstrates a
 variety of example functions and programming techniques for driving both RGB
 and RGB+W NepPixels, including: conditional compilation of RGB or RGB+W code,
 the use of assembly language functions within C code, table look-ups for both
 output gamma correction and pattern display, and and the process of creating
 memory arrays for pattern storage and using them for pattern output.
 
 Use SW2 to select between the different operating modes: colour cyling rainbow,
 shooting 'ion' blob, warming stripes display, random colour fade transitions,
 and pre-set colour mode. In pre-set colour mode, SW3 cycles through red values,
 SW4 cycles through green values, and SW5 cycles through blue values allowing
 any colour to be chosen.
 
 Inspiration for the visualization of the global warming stripes was taken from
 Ed Hawkins and Show your Stripes [https://showyourstripes.info] using global
 temperature data from Berkely Earth.
==============================================================================*/

#include    "xc.h"              // Microchip XC8 compiler include file
#include    "stdint.h"          // Include integer definitions
#include    "stdbool.h"         // Include Boolean (true/false) definitions

#include    "UBMP420.h"         // Include UBMP4.2 constants and functions

// TODO Set linker ROM ranges to 'default,-0-7FF' under "Memory model" pull-down.
// TODO Set linker code offset to '800' under "Additional options" pull-down.

#define RGBW                    // Defined if RGB+W NeoPixels are in the strip
                                // Comment out for RGB-only NeoPixel strips

#define neoLEDs 60              // Total number of physical LEDs in the strip
#define neoDel  16              // Delay between NeoPixel updates in ms

// Operating modes
#define OFF_MODE 0              // Strip is off except for power indicator
#define RAINBOW_MODE 1          // Colour cycling rainbows!
#define IONGUN_MODE 2           // Shooting ion blobs!!
#define WARMINGSTRIPES_MODE 3   // Climate warming stripes :O
#define RANDOM_MODE 4           // Random colour transitions every second
#define COLOUR_MODE 5           // Use the pushbuttons to pick your own colours

extern void neo_shift(unsigned char);
#ifdef RGBW
extern void neo_fill_RGBW(unsigned char);
#else
extern void neo_fill_RGB(unsigned char);
#endif

// RGB colour pixel array variables
unsigned char rPix[neoLEDs];        // Array size can be the same as the total
unsigned char gPix[neoLEDs];        // number of pixels for small strips. Array
unsigned char bPix[neoLEDs];        // sizes are limited by available RAM.

signed char pixIndex = 0;           // Current position in pixel colour arrays
unsigned char pixLEDs = neoLEDs;    // Array size (can be a sub-set of total LEDs)

// RGB colour values
unsigned char rVal = 64;        // Primary RGB colour data values
unsigned char gVal = 32;
unsigned char bVal = 128;

unsigned char rVal2 = 0;        // Secondary RGB colour values for transitions
unsigned char gVal2 = 0;
unsigned char bVal2 = 0;

// White values
#ifdef RGBW
unsigned char wVal = 0;         // Primary white value
unsigned char wVal2 = 0;        // Secondary white value
#endif

// Colour index variables for look-up tables
unsigned char ri; 
unsigned char gi;
unsigned char bi;
#ifdef RGBW
unsigned char wi;
#endif

// Variable definitions for external neo_shift and neo_fill_RGB functions
volatile unsigned char leds;
volatile unsigned char bits;
volatile unsigned char cVal;

// Mode and mode switching variables
unsigned char mode = OFF_MODE;  // Starting operating mode
unsigned char button;           // Button return variable
unsigned char buttonDelay;      // Button delay counter

// Random number generator variables
extern int rand();				// External random number function
extern void srand(unsigned int); // External random seed function

// Button constant definitions
#define NOBUTTON    0
#define MODESEL     1           // Mode selector
#define RBUTTON     2
#define GBUTTON     3
#define BBUTTON     4

// Gamma 1.8 colour value look-up table
const char gamma[256] = {
0, 0, 0, 0, 0, 0, 0, 0,
1, 1, 1, 1, 1, 1, 1, 2,
2, 2, 2, 2, 3, 3, 3, 3,
4, 4, 4, 4, 5, 5, 5, 6,
6, 6, 7, 7, 8, 8, 8, 9,
9, 10, 10, 10, 11, 11, 12, 12,
13, 13, 14, 14, 15, 15, 16, 16,
17, 17, 18, 18, 19, 19, 20, 21,
21, 22, 22, 23, 24, 24, 25, 26,
26, 27, 28, 28, 29, 30, 30, 31,
32, 32, 33, 34, 35, 35, 36, 37,
38, 38, 39, 40, 41, 41, 42, 43,
44, 45, 46, 46, 47, 48, 49, 50,
51, 52, 53, 53, 54, 55, 56, 57,
58, 59, 60, 61, 62, 63, 64, 65,
66, 67, 68, 69, 70, 71, 72, 73,
74, 75, 76, 77, 78, 79, 80, 81,
82, 83, 84, 86, 87, 88, 89, 90,
91, 92, 93, 95, 96, 97, 98, 99,
100, 102, 103, 104, 105, 107, 108, 109,
110, 111, 113, 114, 115, 116, 118, 119,
120, 122, 123, 124, 126, 127, 128, 129,
131, 132, 134, 135, 136, 138, 139, 140,
142, 143, 145, 146, 147, 149, 150, 152,
153, 154, 156, 157, 159, 160, 162, 163,
165, 166, 168, 169, 171, 172, 174, 175,
177, 178, 180, 181, 183, 184, 186, 188,
189, 191, 192, 194, 195, 197, 199, 200,
202, 204, 205, 207, 208, 210, 212, 213,
215, 217, 218, 220, 222, 224, 225, 227,
229, 230, 232, 234, 236, 237, 239, 241,
243, 244, 246, 248, 250, 251, 253, 255
};

// Sine wave look-up table. 120 steps + 60 blanks (makes pretty rainbows)
const char sine[180] = {
0, 0, 1, 2, 3, 4, 6, 9,
11, 14, 17, 21, 25, 29, 33, 37,
42, 47, 53, 58, 64, 70, 76, 82,
88, 95, 101, 108, 115, 121, 128, 134,
140, 147, 154, 160, 167, 173, 179, 185,
191, 197, 202, 208, 213, 218, 222, 226,
231, 234, 238, 241, 244, 246, 249, 251,
252, 253, 254, 255, 255, 255, 254, 253,
252, 251, 249, 246, 244, 241, 238, 234,
231, 226, 222, 218, 213, 208, 202, 197,
191, 185, 179, 173, 167, 160, 154, 147,
140, 134, 127, 121, 115, 108, 101, 95,
88, 82, 76, 70, 64, 58, 53, 47,
42, 37, 33, 29, 25, 21, 17, 14,
11, 9, 6, 4, 3, 2, 1, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0};

// Blues-to-reds Brewer palette - red values
const char brewer_r[20] = {
0x00, 0x08, 0x08, 0x21, 0x42, 0x6b, 0x9e, 0xc6,
0xde, 0xf7, 0xff, 0xff, 0xfe, 0xfc, 0xfc, 0xfb,
0xef, 0xcb, 0xa5, 0x67};

// Blues-to-reds Brewer palette - green values
const char brewer_g[20] = {
0x00, 0x30, 0x51, 0x71, 0x92, 0xae, 0xca, 0xdb,
0xeb, 0xfb, 0xff, 0xf5, 0xe0, 0xbb, 0x92, 0x6a,
0x3b, 0x18, 0x0f, 0x00};

// Blues-to-reds Brewer palette - blue values
const char brewer_b[20] = {
0x00, 0x6b, 0x9c, 0xb5, 0xc6, 0xd6, 0xe1, 0xef,
0xf7, 0xff, 0xff, 0xf0, 0xd2, 0xa1, 0x72, 0x4a,
0x2c, 0x1d, 0x15, 0x0d};

// Blues-to-reds colour palette - red values
const char cooler_r[20] = {
0x01, 0x01, 0x01, 0x01, 0x2a, 0x2c, 0x46, 0x61,
0x89, 0xa9, 0xff, 0xfc, 0xfb, 0xfa, 0xf9, 0xe3,
0xc7, 0x9f, 0x77, 0x50};

// Blues-to-reds colour palette - green values
const char cooler_g[20] = {
0x2a, 0x3a, 0x49, 0x4f, 0x6f, 0x7d, 0x8f, 0xa5,
0xc2, 0xd6, 0xff, 0x9c, 0x74, 0x4c, 0x24, 0x06,
0x05, 0x04, 0x03, 0x02};

// Blues-to-reds colour palette - blue values
const char cooler_b[20] = {
0x4a, 0x63, 0x7c, 0x86, 0x97, 0xa0, 0xaf, 0xc2,
0xd9, 0xe5, 0xff, 0xa2, 0x7d, 0x58, 0x32, 0x13,
0x12, 0x0e, 0x0b, 0x07};

// Climate warming stripes (temperature levels mapped to Brewer palette values)
const char stripes[64] = {
17, 16, 19, 18, 16, 17, 19, 17,
15, 13, 13, 12, 14, 13, 11, 13,
13, 14, 11, 13, 12, 11, 9, 9,
13, 11, 8, 10, 8, 7, 6, 9,
10, 7, 9, 8, 6, 5, 6, 8,
6, 8, 7, 6, 4, 6, 2, 3,
2, 6, 3, 2, 3, 4, 2, 3,
2, 1, 1, 4, 3, 4, 3, 4
};

//// Build climate warming stripes in the pixel array using Brewer palettes
//void warmingStripes(void)
//{
//    for(leds = 0; leds != 64; leds ++)
//    {
//        rPix[leds] = gamma[brewer_r[stripes[leds]]];
//        gPix[leds] = gamma[brewer_g[stripes[leds]]];
//        bPix[leds] = gamma[brewer_b[stripes[leds]]];
//    }
//}

// Build climate warming stripes in the pixel array using alternate palettes
void warmingStripes(void)
{
    for(leds = 0; leds != 64; leds ++)
    {
        rPix[leds] = gamma[cooler_r[stripes[leds]]];
        gPix[leds] = gamma[cooler_g[stripes[leds]]];
        bPix[leds] = gamma[cooler_b[stripes[leds]]];
    }
}

// Build purple 'ion blob' in the pixel array
void blob(void)
{
    rPix[0] = 1;            // Tail first to fire away from output I/O pin
    gPix[0] = 0;
    bPix[0] = 1;
    rPix[1] = 2;
    gPix[1] = 0;
    bPix[1] = 3;
    rPix[2] = 8;
    gPix[2] = 0;
    bPix[2] = 12;
    rPix[3] = 24;
    gPix[3] = 0;
    bPix[3] = 32;
    rPix[4] = 90;
    gPix[4] = 0;
    bPix[4] = 120;
    rPix[5] = 230;
    gPix[5] = 0;
    bPix[5] = 255;
    rPix[6] = 90;
    gPix[6] = 0;
    bPix[6] = 120;
    for(leds = 7; leds != 24; leds ++)  // Black space between blobs
    {
        rPix[leds] = 0;
        gPix[leds] = 0;
        bPix[leds] = 0;        
    }
    wVal = 0;
}

// Shift 8-bits of colour data to NeoPixel output pin
void np_shift(unsigned char col)
{
    for(bits = 8; bits != 0; bits --)
    {
        H1OUT = 1;
        if((col & 0b10000000) == 0)
        {
            H1OUT = 0;
        }
        col = col << 1;
        H1OUT = 0;
    }
}

// Fill all pixels of the NeoPixel strip with the same red, green, and blue 
// (and white) colour values
void np_fill(unsigned char leds)
{
    for(leds; leds != 0; leds --)
    {
        np_shift(gVal);
        np_shift(rVal);
        np_shift(bVal);
#ifdef RGBW
        np_shift(wVal);
#endif
    }
}

// Fill all pixels of the NeoPixel strip with gamma-adjusted red, green, and
// blue colour values
void np_gamma_fill(unsigned char leds)
{
    for(leds; leds != 0; leds --)
    {
        np_shift(gamma[gVal]);
        np_shift(gamma[rVal]);
        np_shift(gamma[bVal]);
#ifdef RGBW
        np_shift(gamma[wVal]);
#endif
    }
}

// Blank all pixels of the NeoPixel strip, except one
void np_off(void)
{
    for(leds = neoLEDs; leds != 0; leds --)
    {
        np_shift(0);
        np_shift(0);
        np_shift(0);
#ifdef RGBW
        np_shift(0);
#endif
    }
    __delay_us(200);
    
    // Leave the porch light on ;)
    np_shift(32);
    np_shift(0);
    np_shift(0);
#ifdef RGBW
    np_shift(0);
#endif
    __delay_us(200);
}

// Cross-fade NeoPixels from the starting colour values (rVal, etc.) to the
// ending colour values (rVal2, etc.) in 16 steps.
void np_crossfade(unsigned char leds)
{
    unsigned char r = rVal;
    unsigned char g = gVal;
    unsigned char b = bVal;
#ifdef RGBW
    signed char w = wVal;
#endif

    // Blend colours for each step by subtracting 1/16 of the staring value and
    // adding 1/16 of the ending value during each step
    for(unsigned char steps = 16; steps != 0; steps--)
    {
        rVal = rVal - (r >> 4) + (rVal2 >> 4);
        gVal = gVal - (g >> 4) + (gVal2 >> 4);
        bVal = bVal - (b >> 4) + (bVal2 >> 4);
#ifdef RGBW
        wVal = wVal - (w >> 4) + (wVal2 >> 4);
#endif
        
        np_fill(neoLEDs);
        __delay_ms(neoDel);
    }
    
    rVal = rVal2;
    gVal = gVal2;
    bVal = bVal2;
#ifdef RGBW
    wVal = wVal2;
#endif
    
    np_fill(neoLEDs);
    __delay_ms(neoDel);
}

// Fill and repeat RGB array values into the number of specified LEDs. The
// pixLEDs (array) size can be smaller or larger than the physical number
// of LEDs. The variable pixIndex stores the starting index of the array fill.
// Setting offset to 0 fills the array starting from the pixIndex address. 
// Positive and negative offsets modify pixIndex and are used to scroll the
// contents of the array.
void np_fill_array(unsigned char leds, signed char offset)
{
    if(pixIndex >= pixLEDs)
    {
        pixIndex = 0;
    }
    signed char tempIndex = pixIndex;
    
	for(leds; leds != 0; leds--)	// Repeat all colour bits for each LED
	{
        np_shift(gPix[pixIndex]);   //Shift RGB array data
        np_shift(rPix[pixIndex]);
        np_shift(bPix[pixIndex]);
#ifdef RGBW
        np_shift(wVal);            // Shift white constant
#endif
        
        pixIndex++;
        if(pixIndex == pixLEDs)
        {
            pixIndex = 0;
        }
	}
    pixIndex = tempIndex - offset;
    if(pixIndex >= pixLEDs)
    {
        pixIndex = pixIndex - pixLEDs;
    }
    if(pixIndex < 0)
    {
        pixIndex = pixIndex + pixLEDs - 1;
    }
}

// Fill RGB array values into the number of specified LEDs.
void np_array(unsigned char leds)
{
    for(unsigned char led = 0; led != leds; led++)
    {
        np_shift(gPix[led]);
        np_shift(rPix[led]);
        np_shift(bPix[led]);
#ifdef RGBW
        np_shift(wVal);
#endif
    }
}

// Read buttons and change mode
unsigned char read_button(void)
{
    if(SW1 == 0)
    {
        RESET();
    }
    else if(SW2 == 0)
    {
        mode ++;                // Next mode
        if(mode == 6)           // If out of modes, switch to off mode
        {
            mode = OFF_MODE;
            np_off(); 
        }
        if(mode == RANDOM_MODE)
        {
            buttonDelay = 1;
        }
        else
        {
            buttonDelay = 25;       // Add mode key repeat delay
        }
    }
    else if(SW3 == 0)
    {
        return(RBUTTON);
    }
    else if(SW4 == 0)
    {
        return(GBUTTON);
    }
    else if(SW5 == 0)
    {
        return(BBUTTON);
    }
    else
    {
        return(NOBUTTON);
    }
}


int main(void)
{
    OSC_config();               // Configure internal oscillator for 48 MHz
    UBMP4_config();             // Configure I/O for on-board UBMP4 devices
    
    H1OUT = 0;                  // Ensure NeoPixel strip is reset
    __delay_us(200);            
	np_off();                   // Blank strip and set power indicator
    
    ri = 60;                    // Pre-set starting sine indices for rainbow
    gi = 0;
    bi = 120;
    
    blob();                     // Pre-load array and set array size for ion gun
    pixLEDs = 24;

    while(1)
    {
        while(mode == OFF_MODE)
        {
            __delay_ms(neoDel);

            if(buttonDelay == 0)
            {
                button = read_button();
            }
            else
            {
                buttonDelay --;
            }
        }

        // Make a colour shifting rainbow pattern using a sine wave table
        while(mode == RAINBOW_MODE)
        {
            rVal = ri;
            gVal = gi;
            bVal = bi;

            for(leds = neoLEDs; leds != 0; leds --)
            {
                np_shift(sine[gVal]);
                gVal ++;
                if(gVal == 180)
                    gVal = 0;
                np_shift(sine[rVal]);
                rVal ++;
                if(rVal == 180)
                    rVal = 0;
                np_shift(sine[bVal]);
                 bVal ++;
                if(bVal == 180)
                    bVal = 0;
               np_shift(0);
            }
            ri ++;
            if(ri == 180)
                ri = 0;
            gi ++;
            if(gi == 180)
                gi = 0;
            bi ++;
            if(bi == 180)
                bi = 0;

            __delay_ms(neoDel);

            if(buttonDelay == 0)
            {
                button = read_button();
                if(mode != RAINBOW_MODE)
                {
                    blob();     // Pre-load array and set size for ion gun mode
                    pixLEDs = 24;
                }
            }
            else
            {
                buttonDelay --;
            }
        }

        // Move the 'ion' array to shoot ion blobs!
        while(mode == IONGUN_MODE)
        {
            np_fill_array(neoLEDs,1);
            __delay_ms(neoDel);

            if(buttonDelay == 0)
            {
                button = read_button();
                if(mode != IONGUN_MODE)
                {
                    warmingStripes();   // Pre-load array and set size for stripes
                    pixLEDs = 60;
                    pixIndex = 0;
                }
            }
            else
            {
                buttonDelay --;
            }
        }
        
        // Display the static warming stripes array
        while(mode == WARMINGSTRIPES_MODE)
        {
            np_fill_array(neoLEDs,0);
            __delay_ms(neoDel);

            if(buttonDelay == 0)
            {
                button = read_button();
            }
            else
            {
                buttonDelay --;
            }
        }
        
        // Randomly choose a new colour and cross-fade to new colour
        while(mode == RANDOM_MODE)
        {
            rVal2 = rand() >> 8;
            gVal2 = rand() >> 8;
            bVal2 = rand() >> 8;
            np_crossfade(neoLEDs);
            __delay_ms(1000);
            
            if(buttonDelay == 0)
            {
                button = read_button();
            }
            else
            {
                buttonDelay --;
            }
        }

        // Allow user to choose their own colour
        while(mode == COLOUR_MODE)
        {
//            np_fill(neoLEDs);           // Regular colour fill
//            neo_fill_RGBW(neoLEDs);     // Assembly code function colour fill
            np_gamma_fill(neoLEDs);     // Gamma-corrected colour fill
            __delay_ms(neoDel);

            if(buttonDelay == 0)
            {
                button = read_button();

                if(button == RBUTTON)
                {
                    rVal ++;
                }
                else if(button == GBUTTON)
                {
                    gVal ++;
                }
                else if(button == BBUTTON)
                {
                    bVal ++;
                }
            }
            else
            {
                buttonDelay --;
            }
        }
    }
}

/* Learn More -- Program Analysis Activities
 * 
 * 1.   Still to come...
 * 
 */

