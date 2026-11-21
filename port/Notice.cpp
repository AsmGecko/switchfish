#include "Notice.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>

#if defined(__SWITCH__)
#include <switch.h>
#endif
#define FONT_DIR SWITCHFISH_ROMFS "/fonts/"

// Drawn in code in the spirit of the game's own dialogs (tan panel, brass frame, pill buttons), so it
// also works when the game files are missing.

namespace {

constexpr float W = 1280, H = 720, PI = 3.14159265f;

SDL_Color Rgba(int r, int g, int b, int a = 255)
{
    return {(Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a};
}

SDL_Color Fade(SDL_Color c, float alpha)
{
    c.a = (Uint8)(c.a * alpha);
    return c;
}

std::string Upper(std::string s)
{
    for (char& c : s)
    {
        c = (char)toupper((unsigned char)c);
    }
    return s;
}

// Collects colored triangles and draws them in one go.
struct Painter
{
    SDL_Renderer* renderer;
    std::vector<SDL_Vertex> verts;

    void Tri(SDL_FPoint a, SDL_FPoint b, SDL_FPoint c, SDL_Color ca, SDL_Color cb, SDL_Color cc)
    {
        verts.push_back({a, ca, {0, 0}});
        verts.push_back({b, cb, {0, 0}});
        verts.push_back({c, cc, {0, 0}});
    }

    // vertical gradient, colors picked by height so stacked shapes line up
    void RoundRect(SDL_FRect r, float radius, SDL_Color top, SDL_Color bottom)
    {
        radius = std::min(radius, std::min(r.w, r.h) / 2);
        auto at = [&](float y)
        {
            float t = std::clamp((y - r.y) / r.h, 0.0f, 1.0f);
            return SDL_Color{(Uint8)(top.r + (bottom.r - top.r) * t), (Uint8)(top.g + (bottom.g - top.g) * t),
                             (Uint8)(top.b + (bottom.b - top.b) * t), (Uint8)(top.a + (bottom.a - top.a) * t)};
        };
        const SDL_FPoint corners[4] = {
            {r.x + r.w - radius, r.y + radius}, {r.x + r.w - radius, r.y + r.h - radius},
            {r.x + radius, r.y + r.h - radius}, {r.x + radius, r.y + radius}};
        std::vector<SDL_FPoint> ring;
        for (int c = 0; c < 4; c++)
        {
            for (int i = 0; i <= 8; i++)
            {
                float a = (c - 1) * PI / 2 + i * (PI / 2) / 8;
                ring.push_back({corners[c].x + std::cos(a) * radius, corners[c].y + std::sin(a) * radius});
            }
        }
        SDL_FPoint center = {r.x + r.w / 2, r.y + r.h / 2};
        for (size_t i = 0; i < ring.size(); i++)
        {
            SDL_FPoint a = ring[i], b = ring[(i + 1) % ring.size()];
            Tri(center, a, b, at(center.y), at(a.y), at(b.y));
        }
    }

    void Circle(float cx, float cy, float radius, SDL_Color top, SDL_Color bottom)
    {
        RoundRect({cx - radius, cy - radius, radius * 2, radius * 2}, radius, top, bottom);
    }

    void Flush()
    {
        if (!verts.empty())
        {
            SDL_RenderGeometry(renderer, nullptr, verts.data(), (int)verts.size(), nullptr, 0);
            verts.clear();
        }
    }
};

// Text with a dark outline, like the game's outlined fonts.
struct Text
{
    SDL_Texture* texture = nullptr;
    int w = 0, h = 0;

    Text() = default;
    Text(const Text&) = delete;
    Text& operator=(const Text&) = delete;

    void Make(SDL_Renderer* renderer, TTF_Font* font, const std::string& s, SDL_Color fill, SDL_Color outline,
              int outlineSize, int wrap = 0)
    {
        if (!font || s.empty())
        {
            return;
        }
        auto render = [&](SDL_Color color)
        {
            return wrap > 0 ? TTF_RenderUTF8_Blended_Wrapped(font, s.c_str(), color, wrap)
                            : TTF_RenderUTF8_Blended(font, s.c_str(), color);
        };
        TTF_SetFontOutline(font, outlineSize);
        SDL_Surface* back = render(outline);
        TTF_SetFontOutline(font, 0);
        SDL_Surface* front = render(fill);
        if (!back || !front)
        {
            SDL_FreeSurface(back);
            SDL_FreeSurface(front);
            return;
        }
        SDL_Rect dst = {outlineSize, outlineSize, front->w, front->h};
        SDL_SetSurfaceBlendMode(front, SDL_BLENDMODE_BLEND);
        SDL_BlitSurface(front, nullptr, back, &dst);
        texture = SDL_CreateTextureFromSurface(renderer, back);
        w = back->w;
        h = back->h;
        SDL_FreeSurface(back);
        SDL_FreeSurface(front);
    }

    void Draw(SDL_Renderer* renderer, float x, float y, float alpha) const
    {
        if (!texture)
        {
            return;
        }
        SDL_SetTextureAlphaMod(texture, (Uint8)(255 * alpha));
        SDL_FRect dst = {x, y, (float)w, (float)h};
        SDL_RenderCopyF(renderer, texture, nullptr, &dst);
    }

    ~Text()
    {
        if (texture)
        {
            SDL_DestroyTexture(texture);
        }
    }
};

struct Button
{
    Text glyph, label;
    SDL_FRect rect;
    bool exits;
    SDL_Color top, bottom;
};

const SDL_Color kOutline = Rgba(52, 30, 8);
const SDL_Color kBrassTop = Rgba(236, 196, 104), kBrassBottom = Rgba(170, 120, 46);
const SDL_Color kTanTop = Rgba(214, 148, 78), kTanBottom = Rgba(186, 114, 50);

void DrawBrass(Painter& p, SDL_FRect r, float radius, float alpha)
{
    p.RoundRect({r.x - 3, r.y - 3, r.w + 6, r.h + 6}, radius + 3, Fade(kOutline, alpha), Fade(kOutline, alpha));
    p.RoundRect(r, radius, Fade(kBrassTop, alpha), Fade(kBrassBottom, alpha));
}

void DrawRivet(Painter& p, float x, float y, float alpha)
{
    p.Circle(x, y + 1, 5, Fade(kOutline, alpha * 0.6f), Fade(kOutline, alpha * 0.6f));
    p.Circle(x, y, 4.5f, Fade(Rgba(255, 232, 160), alpha), Fade(Rgba(150, 100, 34), alpha));
}

void DrawClip(Painter& p, float cx, float cy, bool horizontal, float alpha)
{
    SDL_FRect r = horizontal ? SDL_FRect{cx - 14, cy - 9, 28, 18} : SDL_FRect{cx - 9, cy - 14, 18, 28};
    DrawBrass(p, r, 3, alpha);
}

#if defined(__SWITCH__)
const char* kContinueGlyph = "A";
const char* kExitGlyph = "+";
#else
const char* kContinueGlyph = "Enter";
const char* kExitGlyph = "Esc";
#endif

} // namespace

bool ShowNotice(const Notice& notice)
{
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow("switchfish", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          (int)W, (int)H, SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        SDL_Log("Notice: no renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return notice.canContinue;
    }
    SDL_RenderSetLogicalSize(renderer, (int)W, (int)H);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    TTF_Font* titleFont = TTF_OpenFont(FONT_DIR "Fredoka-SemiBold.ttf", 40);
    TTF_Font* bodyFont = TTF_OpenFont(FONT_DIR "Fredoka-SemiBold.ttf", 27);
    TTF_Font* buttonFont = TTF_OpenFont(FONT_DIR "Fredoka-SemiBold.ttf", 26);
    if (!titleFont || !bodyFont || !buttonFont)
    {
        SDL_Log("Notice: fonts missing in " FONT_DIR ": %s", TTF_GetError());
    }
    if (bodyFont)
    {
        TTF_SetFontWrappedAlign(bodyFont, TTF_WRAPPED_ALIGN_CENTER);
    }

    int choice = -1;
    {
        const float panelW = 760, frame = 14, pad = 56;
        Text title, body;
        title.Make(renderer, titleFont, Upper(notice.title), Rgba(255, 228, 64), kOutline, 3);
        body.Make(renderer, bodyFont, notice.body, Rgba(255, 246, 222), kOutline, 2, (int)(panelW - 2 * pad));

        std::vector<Button> buttons(notice.canContinue ? 2 : 1);
        if (notice.canContinue)
        {
            buttons[0].glyph.Make(renderer, buttonFont, kContinueGlyph, Rgba(255, 255, 255), kOutline, 0);
            buttons[0].label.Make(renderer, buttonFont, "CONTINUE", Rgba(255, 255, 255), kOutline, 2);
            buttons[0].exits = false;
            buttons[0].top = Rgba(110, 206, 255);
            buttons[0].bottom = Rgba(30, 128, 220);
        }
        Button& exitButton = buttons.back();
        exitButton.glyph.Make(renderer, buttonFont, kExitGlyph, Rgba(255, 255, 255), kOutline, 0);
        exitButton.label.Make(renderer, buttonFont, "EXIT", Rgba(255, 255, 255), kOutline, 2);
        exitButton.exits = true;
        exitButton.top = Rgba(190, 120, 240);
        exitButton.bottom = Rgba(112, 48, 176);

        const float buttonH = 56, gap = 28;
        float buttonW = 250;
        for (const Button& b : buttons)
        {
            buttonW = std::max(buttonW, std::max(30.0f, b.glyph.w + 18.0f) + b.label.w + 70);
        }
        const float panelH = pad + 30 + body.h + 44 + buttonH + pad * 0.7f;
        const SDL_FRect panel = {(W - panelW) / 2, (H - panelH) / 2 + 16, panelW, panelH};
        float rowW = buttons.size() * buttonW + (buttons.size() - 1) * gap;
        for (size_t i = 0; i < buttons.size(); i++)
        {
            buttons[i].rect = {(W - rowW) / 2 + i * (buttonW + gap), panel.y + panel.h - pad * 0.7f - buttonH,
                               buttonW, buttonH};
        }

#if defined(__SWITCH__)
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        PadState pad1;
        padInitializeDefault(&pad1);
#endif

        Uint64 start = SDL_GetPerformanceCounter();
        while (choice < 0)
        {
            SDL_Event e;
            while (SDL_PollEvent(&e))
            {
                float tx = -1, ty = -1;
                if (e.type == SDL_QUIT)
                {
                    choice = 0;
                }
                else if (e.type == SDL_KEYDOWN)
                {
                    SDL_Keycode k = e.key.keysym.sym;
                    if (k == SDLK_ESCAPE)
                    {
                        choice = 0;
                    }
                    else if ((k == SDLK_RETURN || k == SDLK_KP_ENTER) && notice.canContinue)
                    {
                        choice = 1;
                    }
                }
                else if (e.type == SDL_FINGERDOWN)
                {
                    tx = e.tfinger.x * W;
                    ty = e.tfinger.y * H;
                }
                else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.which != SDL_TOUCH_MOUSEID)
                {
                    tx = (float)e.button.x;
                    ty = (float)e.button.y;
                }
                for (const Button& b : buttons)
                {
                    if (tx >= b.rect.x && tx < b.rect.x + b.rect.w && ty >= b.rect.y && ty < b.rect.y + b.rect.h)
                    {
                        choice = b.exits ? 0 : 1;
                    }
                }
            }
#if defined(__SWITCH__)
            if (!appletMainLoop())
            {
                choice = 0;
            }
            padUpdate(&pad1);
            u64 down = padGetButtonsDown(&pad1);
            if (down & HidNpadButton_Plus)
            {
                choice = 0;
            }
            else if ((down & HidNpadButton_A) && notice.canContinue)
            {
                choice = 1;
            }
#endif

            float t = (SDL_GetPerformanceCounter() - start) / (float)SDL_GetPerformanceFrequency();
            float alpha = std::min(1.0f, t / 0.25f);

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            Painter p{renderer};
            p.RoundRect({0, 0, W, H}, 0, Rgba(18, 70, 92), Rgba(5, 24, 38));

            // panel: shadow, brass frame, dark seam, tan face
            p.RoundRect({panel.x + 4, panel.y + 12, panel.w, panel.h}, 40, Fade(Rgba(0, 0, 0, 90), alpha), Fade(Rgba(0, 0, 0, 90), alpha));
            DrawBrass(p, panel, 38, alpha);
            SDL_FRect seam = {panel.x + frame - 3, panel.y + frame - 3, panel.w - 2 * frame + 6, panel.h - 2 * frame + 6};
            p.RoundRect(seam, 28, Fade(Rgba(110, 66, 22), alpha), Fade(Rgba(90, 52, 16), alpha));
            SDL_FRect face = {panel.x + frame, panel.y + frame, panel.w - 2 * frame, panel.h - 2 * frame};
            p.RoundRect(face, 25, Fade(kTanTop, alpha), Fade(kTanBottom, alpha));

            for (float fx : {0.22f, 0.78f})
            {
                DrawClip(p, panel.x + panel.w * fx, panel.y + 2, true, alpha);
                DrawClip(p, panel.x + panel.w * fx, panel.y + panel.h - 2, true, alpha);
            }
            for (float fy : {0.3f, 0.7f})
            {
                DrawClip(p, panel.x + 2, panel.y + panel.h * fy, false, alpha);
                DrawClip(p, panel.x + panel.w - 2, panel.y + panel.h * fy, false, alpha);
            }
            DrawRivet(p, face.x + 22, face.y + 22, alpha);
            DrawRivet(p, face.x + face.w - 22, face.y + 22, alpha);

            // title plate on the top edge
            float plateW = std::max(360.0f, title.w + 90.0f), plateH = 62;
            SDL_FRect plate = {(W - plateW) / 2, panel.y - plateH / 2 + 4, plateW, plateH};
            DrawBrass(p, plate, 18, alpha);
            p.RoundRect({plate.x + 6, plate.y + 6, plate.w - 12, plate.h - 12}, 13, Fade(Rgba(196, 160, 70), alpha), Fade(Rgba(150, 112, 40), alpha));

            for (const Button& b : buttons)
            {
                DrawBrass(p, b.rect, b.rect.h / 2, alpha);
                SDL_FRect pill = {b.rect.x + 7, b.rect.y + 7, b.rect.w - 14, b.rect.h - 14};
                p.RoundRect(pill, pill.h / 2, Fade(b.bottom, alpha), Fade(b.bottom, alpha));
                p.RoundRect({pill.x + 2, pill.y + 2, pill.w - 4, pill.h - 4}, pill.h / 2 - 2, Fade(b.top, alpha), Fade(b.bottom, alpha));
                p.RoundRect({pill.x + 14, pill.y + 4, pill.w - 28, pill.h * 0.4f}, pill.h * 0.2f,
                            Fade(Rgba(255, 255, 255, 110), alpha), Fade(Rgba(255, 255, 255, 20), alpha));
                float glyphW = std::max(30.0f, b.glyph.w + 18.0f);
                p.RoundRect({pill.x + 10, pill.y + (pill.h - 30) / 2, glyphW, 30}, 15, Fade(Rgba(30, 20, 40, 150), alpha), Fade(Rgba(30, 20, 40, 150), alpha));
            }
            p.Flush();

            title.Draw(renderer, (W - title.w) / 2, plate.y + (plate.h - title.h) / 2, alpha);
            body.Draw(renderer, (W - body.w) / 2, panel.y + pad + 30, alpha);
            for (const Button& b : buttons)
            {
                SDL_FRect pill = {b.rect.x + 7, b.rect.y + 7, b.rect.w - 14, b.rect.h - 14};
                float glyphW = std::max(30.0f, b.glyph.w + 18.0f);
                b.glyph.Draw(renderer, pill.x + 10 + (glyphW - b.glyph.w) / 2, pill.y + (pill.h - b.glyph.h) / 2, alpha);
                float labelX = pill.x + 10 + glyphW + (pill.w - 10 - glyphW - b.label.w) / 2;
                b.label.Draw(renderer, labelX, pill.y + (pill.h - b.label.h) / 2, alpha);
            }

            SDL_RenderPresent(renderer);
        }
    } // textures belong to the renderer, so they go first

    TTF_CloseFont(titleFont);
    TTF_CloseFont(bodyFont);
    TTF_CloseFont(buttonFont);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return choice == 1;
}
