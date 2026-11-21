#include "WideTank.h"
#include "WinFishCommon.h"
#include "Res.h"
#include "graphics/MemoryImage.h"
#include "SexyAppBase.h"
#include "graphics/GLImage.h"
#include <SDL2/SDL.h>
#include <string>
#include <algorithm>
#include <vector>

using namespace Sexy;

namespace {

MemoryImage* gWide[6] = {};
MemoryImage* gWideMask[6] = {};

// The tank mask is the front strip of the floor (drawn over fish and light bands). Widened from the wide
// backdrop, with the mask's edge transparency carried out into the new parts.
MemoryImage* ExtendMask(SexyAppBase* app, MemoryImage* mask, MemoryImage* wide, int theY)
{
    uint32_t* mbits = mask->GetBits();
    if (!mbits)
    {
        return nullptr;
    }
    const int margin = -TankLeft(), w = mask->mWidth, h = mask->mHeight, ww = w + 2 * margin;
    uint32_t* wbits = wide->GetBits();
    MemoryImage* out = new MemoryImage(app);
    out->Create(ww, h);
    uint32_t* dst = out->GetBits();
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < ww; x++)
        {
            // alpha follows the original: its glass walls moved out to the edges, the middle where it was,
            // the gaps take the column next to the wall. Colour always from the wide backdrop.
            const int glass = 40;
            int mx;
            if (x < glass)
            {
                mx = x;
            }
            else if (x < margin + glass)
            {
                mx = glass;
            }
            else if (x < margin + w - glass)
            {
                mx = x - margin;
            }
            else if (x < ww - glass)
            {
                mx = w - glass - 1;
            }
            else
            {
                mx = x - 2 * margin;
            }
            uint32_t alpha = mbits[y * w + mx] & 0xFF000000u;
            dst[y * ww + x] = alpha | (wbits[(theY + y) * ww + x] & 0x00FFFFFFu);
        }
    }
    out->BitsChanged();
    return out;
}

// Box blur along one axis, in place on an ARGB buffer.
void BoxBlur(std::vector<uint32_t>& px, int w, int h, int radius, bool horizontal)
{
    std::vector<uint32_t> out(px.size());
    int len = horizontal ? w : h, lines = horizontal ? h : w;
    for (int line = 0; line < lines; line++)
    {
        auto at = [&](int i) -> uint32_t& { return horizontal ? px[line * w + i] : px[i * w + line]; };
        for (int i = 0; i < len; i++)
        {
            int sum[3] = {0, 0, 0}, n = 0;
            for (int k = std::max(0, i - radius); k <= std::min(len - 1, i + radius); k++, n++)
            {
                uint32_t c = at(k);
                sum[0] += (c >> 16) & 0xFF;
                sum[1] += (c >> 8) & 0xFF;
                sum[2] += c & 0xFF;
            }
            uint32_t v = 0xFF000000u | ((sum[0] / n) << 16) | ((sum[1] / n) << 8) | (sum[2] / n);
            (horizontal ? out[line * w + i] : out[i * w + line]) = v;
        }
    }
    px.swap(out);
}

// For now: the original stays as painted, the extra width is a darkened, blurred mirror of its edge,
// read as water further from the glass. To be replaced by a proper extension per backdrop.
MemoryImage* Extend(SexyAppBase* app, MemoryImage* src)
{
    const int margin = -TankLeft(), w = src->mWidth, h = src->mHeight, wide = w + 2 * margin;
    uint32_t* bits = src->GetBits();
    if (!bits)
    {
        return nullptr;
    }

    auto wing = [&](bool left)
    {
        std::vector<uint32_t> px(margin * h);
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < margin; x++)
            {
                int sx = left ? margin - 1 - x : w - 1 - x;
                px[y * margin + x] = bits[y * w + sx];
            }
        }
        for (int pass = 0; pass < 3; pass++)
        {
            BoxBlur(px, margin, h, 10, true);
            BoxBlur(px, margin, h, 10, false);
        }
        for (uint32_t& c : px)
        {
            int r = ((c >> 16) & 0xFF) * 6 / 10, g = ((c >> 8) & 0xFF) * 6 / 10, b = (c & 0xFF) * 6 / 10;
            c = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
        return px;
    };
    std::vector<uint32_t> left = wing(true), right = wing(false);

    MemoryImage* out = new MemoryImage(app);
    out->Create(wide, h);
    uint32_t* dst = out->GetBits();
    for (int y = 0; y < h; y++)
    {
        std::copy_n(&left[y * margin], margin, &dst[y * wide]);
        std::copy_n(&bits[y * w], w, &dst[y * wide + margin]);
        std::copy_n(&right[y * margin], margin, &dst[y * wide + margin + w]);
    }
    out->BitsChanged();
    return out;
}

// A piece of the original that goes into the pack image pixel exact (its middle, inside the old glass walls).
struct Piece
{
    int srcX0, srcX1, dstX;
};

// The original is shaded differently near its glass walls than the generated parts. Per row, take the
// colour difference across a seam and carry it into the generated side, fading out over `reach` pixels
// (full strength inside the feather), so the shading ends softly instead of at a straight line.
void MatchSeam(const uint32_t* src, int w, uint32_t* dst, int wide, int h, int srcEdge, int dstEdge, int dir,
    int feather)
{
    const int strip = 12, reach = 90, smooth = 15;
    auto channel = [](uint32_t c, int i) { return (int)((c >> (16 - 8 * i)) & 0xFF); };
    // dir = -1: the generated side is left of dstEdge, +1: right of it. srcEdge/dstEdge are the first
    // column of the original piece on that side.
    std::vector<float> diff(h * 3), smoothed(h * 3);
    for (int y = 0; y < h; y++)
    {
        for (int i = 0; i < 3; i++)
        {
            float sum = 0;
            for (int k = 0; k < strip; k++)
            {
                sum += channel(src[y * w + srcEdge - dir * k], i) - channel(dst[y * wide + dstEdge - dir * k], i);
            }
            diff[y * 3 + i] = sum / strip;
        }
    }
    for (int y = 0; y < h; y++)
    {
        for (int i = 0; i < 3; i++)
        {
            float sum = 0;
            int n = 0;
            for (int k = std::max(0, y - smooth); k <= std::min(h - 1, y + smooth); k++, n++)
            {
                sum += diff[k * 3 + i];
            }
            smoothed[y * 3 + i] = sum / n;
        }
    }
    for (int y = 0; y < h; y++)
    {
        for (int d = -feather; d < reach; d++)
        {
            int x = dstEdge + dir * (d + 1);
            if (x < 0 || x >= wide)
            {
                break;
            }
            float weight = d < 0 ? 1.0f : 1.0f - (float)d / reach;
            uint32_t& c = dst[y * wide + x];
            int out[3];
            for (int i = 0; i < 3; i++)
            {
                out[i] = std::clamp((int)(channel(c, i) + smoothed[y * 3 + i] * weight), 0, 255);
            }
            c = (c & 0xFF000000u) | (out[0] << 16) | (out[1] << 8) | out[2];
        }
    }
}

void PastePiece(const uint32_t* src, int w, uint32_t* dst, int wide, int h, const Piece& p, int feather)
{
    const int width = p.srcX1 - p.srcX0;
    const bool seamLeft = p.dstX > 0, seamRight = p.dstX + width < wide;
    if (seamLeft)
    {
        MatchSeam(src, w, dst, wide, h, p.srcX0, p.dstX, -1, feather);
    }
    if (seamRight)
    {
        MatchSeam(src, w, dst, wide, h, p.srcX1 - 1, p.dstX + width - 1, +1, feather);
    }
    for (int y = 0; y < h; y++)
    {
        for (int i = 0; i < width; i++)
        {
            int edge = std::min(seamLeft ? i : feather, seamRight ? width - 1 - i : feather);
            uint32_t a = src[y * w + p.srcX0 + i], &b = dst[y * wide + p.dstX + i];
            if (edge >= feather)
            {
                b = a;
                continue;
            }
            int t = (edge + 1) * 256 / (feather + 1);
            auto mix = [&](int shift) { return (((a >> shift) & 0xFF) * t + ((b >> shift) & 0xFF) * (256 - t)) >> 8; };
            b = 0xFF000000u | (mix(16) << 16) | (mix(8) << 8) | mix(0);
        }
    }
}

// The pack image (the player's own, made with an image model from their backdrop, see scripts/widescreen)
// supplies the gaps; everything else is the original, pixel exact.
MemoryImage* FromPack(SexyAppBase* app, MemoryImage* src, int theBackgroundId)
{
    // a pack next to the game files wins over the one built into the NRO
    std::string path = "widescreen/aquarium" + std::to_string(theBackgroundId);
    MemoryImage* pack = app->GetImage(path);
    if (!pack)
    {
        path = SWITCHFISH_ROMFS "/" + path;
        pack = app->GetImage(path);
    }
    if (!pack)
    {
        return nullptr;
    }
    const int margin = -TankLeft(), w = src->mWidth, h = src->mHeight, wide = w + 2 * margin;
    if (pack->mWidth != wide || pack->mHeight != h)
    {
        SDL_Log("wide tank: %s is %dx%d, expected %dx%d", path.c_str(), pack->mWidth, pack->mHeight, wide, h);
        delete pack;
        return nullptr;
    }
    const int glass = 40, feather = 12;
    // the image model draws the glass walls at the new edges itself, so only the middle comes back
    const Piece pieces[] = {
        {glass, w - glass, margin + glass},
    };
    for (const Piece& p : pieces)
    {
        PastePiece(src->GetBits(), w, pack->GetBits(), wide, h, p, feather);
    }
    pack->BitsChanged();
    return pack;
}

} // namespace

void BuildWideBackdrops(SexyAppBase* app)
{
    for (int i = 0; i < 6; i++)
    {
        delete gWide[i];
        gWide[i] = nullptr;
        delete gWideMask[i];
        gWideMask[i] = nullptr;
        if (TankWidth() <= 640)
        {
            continue;
        }
        MemoryImage* src = dynamic_cast<MemoryImage*>(GetImageById(IMAGE_AQUARIUM1_ID + i));
        if (src)
        {
            gWide[i] = FromPack(app, src, i + 1);
            if (!gWide[i])
            {
                gWide[i] = Extend(app, src);
            }
        }
        if (!gWide[i])
        {
            SDL_Log("wide tank: no pixels for backdrop %d", i + 1);
            continue;
        }
        MemoryImage* mask = dynamic_cast<MemoryImage*>(GetImageById(IMAGE_TANKMASK1_ID + i));
        gWideMask[i] = mask ? ExtendMask(app, mask, gWide[i], 365) : nullptr;
    }
}

Image* WideTankMask(int theBackgroundId)
{
    if (theBackgroundId < 1 || theBackgroundId > 6)
    {
        return nullptr;
    }
    return gWideMask[theBackgroundId - 1];
}

Image* WideBackdrop(int theBackgroundId)
{
    if (theBackgroundId < 1 || theBackgroundId > 6)
    {
        return nullptr;
    }
    return gWide[theBackgroundId - 1];
}
