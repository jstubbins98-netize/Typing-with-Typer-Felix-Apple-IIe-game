; Apple IIe vertical-blank status. Either display polarity produces one
; full scan interval per call; no interrupt handler or OS clock needed.
.export _wait_frame, _tone
.export _key_read, _key_flush, _key_held
.segment "CODE"
; Explicit bus accesses: never rely on a discarded volatile C expression.
; Return values use the cc65 A/X convention. Only consume a fresh strobe.
_key_read:
    lda $C000
    bpl @none
    bit $C010
    and #$7F
    ldx #0
    rts
@none:
    lda #0
    tax
    rts

_key_flush:
    bit $C010
    rts

; Apple IIe AKD: bit 7 is high while any key is physically held.
; Reading C010 also drains repeats while waiting for release.
_key_held:
    lda $C010
    and #$80
    ldx #0
    rts

_wait_frame:
@high:
    bit $C019
    bpl @high
@low:
    bit $C019
    bmi @low
    rts

; Short, bounded speaker tone. fastcall unsigned char period arrives in A.
; Twelve half waves: under 7ms at the largest period used by the game.
_tone:
    sta @delay+1
    ldy #12
@wave:
    bit $C030
@delay:
    ldx #40
@loop:
    dex
    bne @loop
    dey
    bne @wave
    rts