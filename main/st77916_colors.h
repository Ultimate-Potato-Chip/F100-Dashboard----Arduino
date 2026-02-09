/**
 * ST77916 Color Fix Header
 *
 * The ST77916 QSPI display has a cyclic color rotation: R→B, G→R, B→G
 * This means: send RED shows BLUE, send GREEN shows RED, send BLUE shows GREEN
 *
 * To correct: for desired (R,G,B), we send (B,R,G) in the RGB positions
 * Formula: 0xRRGGBB → 0xBBRRGG
 */

#ifndef ST77916_COLORS_H
#define ST77916_COLORS_H

// Cyclic color rotation fix: 0xRRGGBB -> 0xBBRRGG
// To show R: send G, to show G: send B, to show B: send R
#define ST77916_FIX_COLOR(c) ((((c) & 0x0000FF) << 16) | (((c) & 0xFF0000) >> 8) | (((c) & 0x00FF00) >> 8))

// Common colors pre-fixed for ST77916 (what to SEND to get the color)
#define ST77916_RED       0x00FF00  // Send GREEN to display RED
#define ST77916_GREEN     0x0000FF  // Send BLUE to display GREEN
#define ST77916_BLUE      0xFF0000  // Send RED to display BLUE
#define ST77916_WHITE     0xFFFFFF  // Symmetric - no change
#define ST77916_BLACK     0x000000  // Symmetric - no change
#define ST77916_YELLOW    0x00FFFF  // Send CYAN to display YELLOW
#define ST77916_CYAN      0xFFFF00  // Send YELLOW to display CYAN
#define ST77916_MAGENTA   0xFF00FF  // Symmetric - no change

#endif /* ST77916_COLORS_H */