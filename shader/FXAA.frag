#version 420

layout (location = 0) out vec4 fragColor;

in vec2 position;

layout (binding  = 0) uniform sampler2D imageTex;

uniform vec2 pixelSize;

/* 0 for default antialiasing
 * 1 for edge visualization
 */
uniform int renderMode;
#define SHOW_EDGES 1

/* Edge detection defines */
#define EDGE_THRESHOLD 0.125

/* Sub-pixel aliasing defines */
#define SUBPIX_TRIM 0.35
#define SUBPIX_TRIM_SCALE (1.0 / (1.0 - SUBPIX_TRIM))
#define SUBPIX_CAP 0.75

/* End-of-edge Search */
#define SEARCH_STEPS 8
#define SEARCH_THRESHOLD 0.25

/* General defines */
#define texLod0(t, uv) textureLod(t, uv, 0.0)

vec3 fxaa()
{
    /* Retrieve rgb colors of the 3x3 region */
    vec3 rgbP  = texLod0(imageTex, position).rgb;
    vec3 rgbN  = texLod0(imageTex, position + vec2(0.0,   pixelSize.y)).rgb;
    vec3 rgbNW = texLod0(imageTex, position + vec2(- pixelSize.x, pixelSize.y)).rgb;
    vec3 rgbW  = texLod0(imageTex, position + vec2(- pixelSize.x, 0.0)).rgb;
    vec3 rgbSW = texLod0(imageTex, position + vec2(- pixelSize.x, - pixelSize.y)).rgb;
    vec3 rgbS  = texLod0(imageTex, position + vec2(0.0, - pixelSize.y)).rgb;
    vec3 rgbSE = texLod0(imageTex, position + vec2(  pixelSize.x, - pixelSize.y)).rgb;
    vec3 rgbE  = texLod0(imageTex, position + vec2(  pixelSize.x, 0.0)).rgb;
    vec3 rgbNE = texLod0(imageTex, position + vec2(  pixelSize.x, pixelSize.y)).rgb;

    vec3  luma   = vec3(0.299, 0.587, 0.114);
    float lumaP  = dot(rgbP, luma);
    float lumaN  = dot(rgbN, luma);
    float lumaNW = dot(rgbNW, luma);
    float lumaW  = dot(rgbW, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaS  = dot(rgbS, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaE  = dot(rgbE, luma);
    float lumaNE = dot(rgbNE, luma);

    float lumaMin = min(lumaP, min(min(lumaN, lumaW), min(lumaS, lumaE)));
    float lumaMax = max(lumaP, max(max(lumaN, lumaW), max(lumaS, lumaE)));

    /* Local contrast check using the pixel and its North, South, East and West neighbors. */
    float range = lumaMax - lumaMin;
    if (range < lumaMax * EDGE_THRESHOLD )
        return rgbP;

    vec3 rgbL = rgbN + rgbW + rgbP + rgbE + rgbS;
    rgbL += rgbNW + rgbNE + rgbSW + rgbSE;
    rgbL *= vec3(1.0 / 9.0);

    /*
     * -----  Sub-pixel aliasing test  -----
     * The ratio of pixel contrast to local contrast is used to detect sub-pixel aliasing.
     *
     * Pixel contrast is estimated as the absolute difference in pixel luma
     * from a lowpass luma: (N + W + E + S) / 4
     *
     * This factor is used to mix rgbL (and rgbF) at the end of the algorithm.
     */
    float lumaL  = (lumaN + lumaW + lumaS + lumaE) * 0.25;
    float rangeL = abs(lumaL - lumaP);
    float blendL = max(0.0, (rangeL / range) - SUBPIX_TRIM) * SUBPIX_TRIM_SCALE;
    blendL = min(SUBPIX_CAP, blendL);

    // Turn sub-pixel aliasing on full force:
    //blendL = rangeL / range;

    /*
     * ------  Vertical/Horizontal edge test  -----
     *
     * This decides whether a vertical or horizontal edge should be processed.
     * Takes a weighted average magnitude of the high-pass values for the 3x3
     * neighborhood as an indication of local edge amount.
     */
    float edgeVert =
            abs((0.25 * lumaNW) + (-0.5 * lumaN) + (0.25 * lumaNE)) +
            abs((0.50 * lumaW)  + (-1.0 * lumaP) + (0.50 * lumaE) ) +
            abs((0.25 * lumaSW) + (-0.5 * lumaS) + (0.25 * lumaSE));
    float edgeHorz =
            abs((0.25 * lumaNW) + (-0.5 * lumaW) + (0.25 * lumaSW)) +
            abs((0.50 * lumaN)  + (-1.0 * lumaP) + (0.50 * lumaS) ) +
            abs((0.25 * lumaNE) + (-0.5 * lumaE) + (0.25 * lumaSE));
    bool horzSpan = edgeHorz >= edgeVert;

    /* For debugging visualization */
    if (renderMode == SHOW_EDGES && horzSpan)
        return vec3(1.0, 0.75, 0.0);
    else if (renderMode == SHOW_EDGES)
        return vec3(0.0, 0.5, 1.0);

    /* Vertical and horizontal edges will be handled the same,
     * sp just use the same variables */
    float luma0, luma1;
    float lengthSign;

    if (horzSpan) {
        luma0 = lumaN;
        luma1 = lumaS;
        lengthSign = - pixelSize.y;
    } else {
        luma0 = lumaW;
        luma1 = lumaE;
        lengthSign = - pixelSize.x;
    }
    float gradient0 = abs(luma0 - lumaP);
    float gradient1 = abs(luma1 - lumaP);
    luma0 = (luma0 + lumaP) * 0.5;
    luma1 = (luma1 + lumaP) * 0.5;

    /* Pair the pixel with its highest contrast neighbor 90 degrees to the edge. */
    if (gradient0 < gradient1) {
        luma0 = luma1;
        gradient0 = gradient1;
        lengthSign *= -1.0;
    }

    /* Calculate the interpolated position of the 'pair' */
    vec2 posNeg;
    posNeg.x = position.x + (horzSpan ? 0.0 : lengthSign * 0.5);
    posNeg.y = position.y + (horzSpan ? lengthSign * 0.5 : 0.0);

    /* Control when to stop searching */
    gradient0 *= SEARCH_THRESHOLD;

    /*
     * ------  End-of-edge Search  -----
     *
     * Search along the edge in the positive and negative direction until a
     * search limit or end-of-edge is reached.
     */
    vec2 posPos = posNeg;
    vec2 offset = horzSpan ? vec2(pixelSize.x, 0.0) : vec2(0.0, pixelSize.y);

    float lumaEndNeg = luma0;
    float lumaEndPos = luma0;

    /* posNeg will go West/South; posPos will go East/North */
    posNeg += offset * vec2(-1.0, -1.0);
    posPos += offset * vec2(1.0, 1.0);

    bool doneNeg = false;
    bool donePos = false;

    /* Use a loop for both directions to avoid branching */
    for(int i=0; i < SEARCH_STEPS; ++i) {
        if (!doneNeg)
            lumaEndNeg = dot(texLod0(imageTex, posNeg).rgb, luma);
        if (!donePos)
            lumaEndPos = dot(texLod0(imageTex, posPos).rgb, luma);

        doneNeg = doneNeg || (abs(lumaEndNeg - luma0) >= gradient0);
        donePos = donePos || (abs(lumaEndPos - luma0) >= gradient0);
        if (doneNeg && donePos)
            break;
        if (!doneNeg)
            posNeg -= offset;
        if (!donePos)
            posPos += offset;
    }

    /* Calculate the distance in positive and negative direction
     * which make up the 'span' */
    float dstNeg, dstPos;

    if (horzSpan) {
        dstNeg = position.x - posNeg.x;
        dstPos = posPos.x - position.x;
    } else {
        dstNeg = position.y - posNeg.y;
        dstPos = posPos.y - position.y;
    }

    bool direction = dstNeg < dstPos;
    float lumaEnd = direction ? lumaEndNeg : lumaEndPos;

    /* Check if pixel is in section of span which gets no filtering.
     * Refer to the original whitepaper and the reference implementation
     * for an explanation of the different cases.
     */
    if (((lumaP - luma0) < 0.0) == ((lumaEnd - luma0) < 0.0)) {
        /* Do subpixel anti-aliasing */
        return mix(rgbL, rgbP, blendL);
    }


    /* Compute offset and retrieve a new rgb value (relying on bilinear filtering) */
    float spanLength = (dstPos + dstNeg);
    float dst = direction ? dstNeg : dstPos;
    float subPixelOffset = (0.5 + (dst * (-1.0 / spanLength))) * lengthSign;

    vec3 rgbF = texLod0(imageTex, vec2(position.x + (horzSpan ? 0.0 : subPixelOffset),
                                       position.y + (horzSpan ? subPixelOffset : 0.0))).rgb;

    // No subpixel aliasing
    //return rgbF;

    // blend result using the subpixel aliasing ratio
    return mix(rgbL, rgbF, blendL);
}

void main(void)
{
    fragColor = vec4(fxaa(), 1.0);
	//fragColor = texLod0(imageTex, position);
}
