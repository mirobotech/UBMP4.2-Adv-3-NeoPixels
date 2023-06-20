;UBMP4 NeoPixel assembly code functions
			   
    PROCESSOR   16F1459
    
#include <xc.inc>

NP_PIN	equ 0		    ;UBMP4 H1 output pin bit position in LATC (change
			    ;to select a different NeoPixel output pin)

;Declare global neopixel functions
GLOBAL _neo_shift, _neo_fill_RGBW, _neo_fill_RGB

;Declare global neopixel variables
GLOBAL _cVal, _bits, _gVal, _rVal, _bVal, _wVal, _leds

;Place code into the neopixel psect
PSECT code		    ;Automatically link code
;PSECT neopixel,local,class=CODE,delta=2	    ;More detailed PSECT inking options

;Shift 8 bit colour byte as NeoPixel data pulses
_neo_shift:
    movwf   _cVal	    ;Copy pixel colour data byte before serial shifting
    movlw   8		    ;Set data bit counter
    movwf   _bits
neoShiftLoop:
    bsf	    LATC,NP_PIN	    ;Start data bit pulse
    nop
    nop
    btfss   _cVal,7	    ;Read data bit from MSB. If MSB == 0, end the data
    bcf	    LATC,NP_PIN	    ;bit pulse (~333us bit length)
    nop
    decf    _bits,f	    ;Count down data bits
    bcf	    LATC,NP_PIN	    ;End data bit pulse (~583us length) for MSB == 1
    btfsc   STATUS,STATUS_Z_POSITION
    return		    ;Return if no more data bits
    lslf    _cVal,f	    ;Left shift next data bit into MSB
    nop
    goto    neoShiftLoop

;Fill RGBW NeoPixel strip with constant g,r,b,w values
_neo_fill_RGBW:
    BANKSEL _leds
    movwf   BANKMASK(_leds) ;Store LED pixel count
fillRGBWLoop:
    movf    _gVal,w	    ;Shift green bits
    fcall   _neo_shift
    
    movf    _rVal,w	    ;Shift red bits
    fcall   _neo_shift

    movf    _bVal,w	    ;Shift blue bits
    fcall   _neo_shift

    movf    _wVal,w	    ;Shift white bits
    fcall   _neo_shift
    
    decfsz  _leds,f	    ;Repeat for all LEDs in the strip
    goto    fillRGBWLoop
    
    return

;Fill RGB NeoPixel strip with constant g,r,b values
_neo_fill_RGB:
    BANKSEL _leds
    movwf   BANKMASK(_leds) ;Store LED pixel count
fillRGBLoop:
    movf    _gVal,w	    ;Shift green bits
    fcall   _neo_shift
    
    movf    _rVal,w	    ;Shift red bits
    fcall   _neo_shift

    movf    _bVal,w	    ;Shift blue bits
    fcall   _neo_shift

    decfsz  _leds,f	    ;Repeat for all LEDs in the strip
    goto    fillRGBLoop
    
    return

    END
