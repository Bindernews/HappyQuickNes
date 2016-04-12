#include "hqn_surface.h"
#include <cstring>
#include <cmath>

namespace hqn
{

// const float pi = 3.14159265359;
const float pi = 3.14159265359f;

// Copied from http://forum.devmaster.net/t/fast-and-accurate-sine-cosine/9648
float fastSin(float x)
{
    const float B = 4/pi;
    const float C = -4/(pi*pi);

    float y = B * x + C * x * abs(x);

    //  const float Q = 0.775;
    const float P = 0.225f;

    y = P * (y * abs(y) - y) + y;   // Q * y + P * y * abs(y)
    return y;
}

float fastCos(float x)
{
    return fastSin(x - pi / 2);
}

Surface::Surface(size_t w, size_t h)
{
    m_pixels = new Color[w * h];
    // initialize all pixels to transparent black
    memset(m_pixels, 0, sizeof(Color) * w * h);
    // set default colors
    m_width = (int)w;
    m_height = (int)h;
    setBlendMode(HQN_BLENDMODE_BLEND);
}

Surface::~Surface()
{
    delete[] m_pixels;
}

void Surface::drawRect(int x, int y, size_t w, size_t h, Color color)
{
    int x2 = x + w;
    int y2 = y + h;
    int x1Bound = boundLeft(x);
    int y1Bound = boundTop(y) + 1;
    int x2Bound = boundRight(x2) - 1;
    int y2Bound = boundBottom(y2);
    // draw vertical lines
    if (x >= 0)
    {
        for (int yy = y1Bound; yy < y2Bound; yy++)
            rawset(x, yy, color);
    }
    if (x2 < (int)m_width)
        for (int yy = y1Bound; yy < y2Bound; yy++)
            rawset(x2, yy, color);
    // draw horizontal lines
    if (y >= 0)
        for (int xx = x1Bound; xx < x2Bound; xx++)
            rawset(xx, y, color);
    if (y2 < (int)m_height)
        for (int xx = x1Bound; xx < x2Bound; xx++)
            rawset(xx, y2, color);
}

void Surface::fillRect(int x, int y, size_t w, size_t h, Color fg, Color bg)
{
    // draw the outline
    drawRect(x, y, w, h, fg);
    // now fill in the middle
    int x2 = boundRight(x + w - 1);
    int y2 = boundBottom(y + h - 1);
    x = boundLeft(x);
    y = boundTop(y);
    for (int yy = y + 1; yy < y2; yy++)
        for (int xx = x + 1; xx < x2; xx++)
            rawset(xx, yy, bg);
}


#define SWAP(a, b, temp) temp = a; a = b; b = temp
void Surface::fastLine(int x1, int y1, int x2, int y2, Color color)
{
    int temp;
    // Make sure x1 <= x2
    if (x1 > x2)
    {
        SWAP(x1, x2, temp);
        SWAP(y1, y2, temp);
    }
    // If any of the points are outside the surface then don't draw
    if (x1 < 0 || y1 < 0 || y1 >= m_height
        || x2 >= m_width || y2 < 0 || y2 >= m_height)
        return;

    // special case
    if (x1 == x2)
    {
        // safety checks
        if (y1 > y2)
        {
            SWAP(y1, y2, temp);
        }
        for (int y = y1; y <= y2; y++)
            rawset(x1, y, color);
        // that's all folks
    }
    else
    {
        const int deltaX = x2 - x1;
        const int deltaY = y2 - y1;
        // absolute value of the slope of the line
        const float deltaErr = std::abs((float)deltaY / deltaX);
        // calculate the sign of delta Y
        const int signDY = deltaY ? deltaY / abs(deltaY) : 0;
        float error = 0;
        int y = y1;
        for (int x = x1; x < x2; x++)
        {
            rawset(x, y, color);
            error += deltaErr;
            while (error >= 0.5)
            {
                rawset(x, y, color);
                y += signDY;
                error -= 1.0f;
            }
        }
    }
}

void Surface::safeLine(int x1, int y1, int x2, int y2, Color color)
{
    // can be used a lot
    int temp;
    // Make sure x1 <= x2
    if (x1 > x2)
    {
        SWAP(x1, x2, temp);
        SWAP(y1, y2, temp);
    }
    // Special case
    if (x1 == x2)
    {
        if (y1 > y2)
        {
            SWAP(y1, y2, temp);
        }
        int y1Bound = boundTop(y1);
        int y2Bound = boundBottom(y2);
        for (int y = y1Bound; y < y2Bound; y++)
            rawset(x1, y, color);
        // that's all folks
    }
    else
    {
        // Prep the variables
        // Remember y = mx + b
        const int deltaX = x2 - x1;
        const int deltaY = y2 - y1;
        const float slope = (float)deltaY / deltaX;
        const float deltaErr = std::abs(slope);
        const int signDY = deltaY ? deltaY / std::abs(deltaY) : 0;

        // Ensure the line is inside the bounds
        if (x1 < 0 || y1 < 0 || y2 < 0)
        {       
        }
        float error = 0;
        int y = y1;
        for (int x = x1; x < x2; x++)
        {
            rawset(x, y, color);
            error += deltaErr;
            while (error >= 0.5)
            {
                // safe setPixel instead of rawset
                setPixel(x, y, color);
                y += signDY;
                error -= 1.0f;
            }
        }
    }
}

void Surface::setPixel(int x, int y, Color color)
{
    if (x < 0 || y < 0 || x >= (int)m_width || y >= (int)m_height)
    {
        return;
    }
    else
    {
        rawset(x, y, color);
    }
}

void Surface::setBlendMode(BlendMode mode)
{
    switch(mode) 
    {
    case HQN_BLENDMODE_NONE:
        m_blendFunc = &blendNone;
        break;
    case HQN_BLENDMODE_BLEND:
        m_blendFunc = &blendBlend;
        break;
    case HQN_BLENDMODE_ADD:
        m_blendFunc = &blendAdd;
        break;
    case HQN_BLENDMODE_MOD:
        m_blendFunc = &blendMod;
        break;
    default:
        // skip setting m_blend
        return;
    }
    m_blend = mode;
}

BlendMode Surface::getBlendMode() const
{
    return m_blend;
}


#define BYTE(exp) (uint8_t)((exp) / 255)
Color Surface::blendBlend(Color src, Color dst)
{
    const uint8_t invA = 255 - src.a;
    dst.r = BYTE(src.r * src.a + dst.r * invA);
    dst.g = BYTE(src.g * src.a + dst.g * invA);
    dst.b = BYTE(src.b * src.a + dst.b * invA);
    dst.a = BYTE(src.a + (dst.a * invA));
    return dst;
}

Color Surface::blendNone(Color src, Color dst)
{
    return src;
}

Color Surface::blendAdd(Color src, Color dst)
{
    dst.r = BYTE(src.r * src.a + dst.r);
    dst.g = BYTE(src.g * src.a + dst.g);
    dst.b = BYTE(src.g * src.a + dst.b);
    return dst;
}

Color Surface::blendMod(Color src, Color dst)
{
    dst.r = BYTE(src.r * dst.r);
    dst.g = BYTE(src.g * dst.g);
    dst.b = BYTE(src.b * dst.b);
    return dst;
}

}