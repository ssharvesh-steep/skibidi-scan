#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>

#include "nob.h"


int dirs[][2] = {
    {-1,  0},
    { 1,  0},
    { 0, -1},
    { 0,  1},
    {-1, -1},
    { 1,  1},
    {-1,  1},
    { 1, -1},
};

#define MINPTS (ARRAY_LEN(dirs))

typedef struct {
    int x, y;
} Coordinate;

Coordinate coordinate(int x, int y)
{
    return (Coordinate) {x, y};
}

typedef struct {
    Coordinate *items;
    size_t count;
    size_t capacity;
} Coordinates;

typedef struct {
    int *items;
    size_t width;
    size_t height;
} Labels;

typedef struct {
    Color *items;
    size_t width;
    size_t height;
} Pixels;

#define MAT_CELL(mat, x, y) (mat)->items[(y)*(mat)->width + (x)]

#define MAT_RESIZE(mat, _width, _height) \
    do { \
        (mat)->items = realloc((mat)->items, sizeof(*(mat)->items)*(_width)*(_height)); \
        (mat)->width = (_width); \
        (mat)->height = (_height); \
    } while(0)

#define MAT_FILL(mat, value) \
    do { \
        for (size_t y = 0; y < (mat)->height; ++y) { \
            for (size_t x = 0; x < (mat)->width; ++x) { \
                MAT_CELL((mat), x, y) = (value); \
            } \
        } \
    } while(0)

#define MAT_CONTAINS(mat, x, y) (0 <= (x) && (size_t)(x) < (mat)->width && 0 <= (y) && (size_t)(y) < (mat)->height)

float angular_distance(float a, float b)
{
    a = fmodf(a/360.0, 1.0);
    b = fmodf(b/360.0, 1.0);
    float mn = fminf(a, b);
    float mx = fmaxf(a, b);
    mx -= mn;
    if (mx <= 0.5) return mx;
    return 1 - mx;
}

float hsv_distance(Vector3 a, Vector3 b)
{
    float dx = angular_distance(a.x, b.x);
    float dy = fabsf(a.y - b.y);
    float dz = fabsf(a.z - b.z);
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

Color all_colors[] = {
    LIME,
    SKYBLUE,
    PURPLE,
    RED,
    BEIGE,
    GOLD,
    BLUE,
    GREEN,
    DARKBLUE,
    VIOLET,
    YELLOW,
    DARKBROWN,
    MAGENTA,
    DARKPURPLE,
    ORANGE,
    DARKGREEN,
    BROWN,
    PINK,
    MAROON,
};

size_t count_neighbors(Pixels pixels, int cx, int cy, float r)
{
    size_t count = 0;
    for (size_t i = 0; i < ARRAY_LEN(dirs); ++i) {
        Vector3 target = ColorToHSV(MAT_CELL(&pixels, cx, cy));
        int nx = cx + dirs[i][0];
        int ny = cy + dirs[i][1];
        if (MAT_CONTAINS(&pixels, nx, ny)) {
            Vector3 nbor = ColorToHSV(MAT_CELL(&pixels, nx, ny));
            if (hsv_distance(target, nbor) <= r) {
                count += 1;
            }
        }
    }
    return count;
}

void cluter_image(Image image, float r, Texture2D *texture)
{
    static Pixels pixels = {0};
    static Labels labels = {0};
    static Coordinates wave = {0};
    static Coordinates next = {0};

    if (pixels.items) UnloadImageColors(pixels.items);
    pixels.items = LoadImageColors(image);
    pixels.width = image.width;
    pixels.height = image.height;
    MAT_RESIZE(&labels, image.width, image.height);
    MAT_FILL(&labels, -1);
    if (labels.width > 0 && labels.height > 0) {
        int label = 1;
        for (;; label += 1) {
            printf("label = %d\n", label);

            wave.count = 0;
            bool found = false;
            for (size_t y = 0; !found && y < labels.height; ++y) {
                for (size_t x = 0; !found && x < labels.width; ++x) {
                    if (MAT_CELL(&labels, x, y) < 0) {
                        if (count_neighbors(pixels, x, y, r) + 1 < MINPTS) {
                            MAT_CELL(&labels, x, y) = 0;
                        } else {
                            da_append(&wave, coordinate(x, y));
                            MAT_CELL(&labels, x, y) = label;
                            found = true;
                        }
                    }
                }
            }

            if (!found) break;

            while (wave.count > 0) {
                next.count = 0;
                da_foreach(Coordinate, c, &wave) {
                    if (count_neighbors(pixels, c->x, c->y, r) + 1 < MINPTS) continue;
                    for (size_t i = 0; i < ARRAY_LEN(dirs); ++i) {
                        Vector3 target = ColorToHSV(MAT_CELL(&pixels, c->x, c->y));
                        int nx = c->x + dirs[i][0];
                        int ny = c->y + dirs[i][1];
                        if (MAT_CONTAINS(&labels, nx, ny) && MAT_CELL(&labels, nx, ny) <= 0) {
                            Vector3 nbor = ColorToHSV(MAT_CELL(&pixels, nx, ny));
                            if (hsv_distance(target, nbor) <= r) {
                                MAT_CELL(&labels, nx, ny) = label;
                                da_append(&next, coordinate(nx, ny));
                            }
                        }
                    }
                }
                swap(Coordinates, wave, next);
            }
        }
        for (size_t y = 0; y < labels.height; ++y) {
            for (size_t x = 0; x < labels.width; ++x) {
                if (MAT_CELL(&labels, x, y) == 0) {
                    MAT_CELL(&pixels, x, y) = BLACK;
                } else {
                    MAT_CELL(&pixels, x, y) = all_colors[MAT_CELL(&labels, x, y)%ARRAY_LEN(all_colors)];
                }
            }
        }
        UnloadTexture(*texture);
        *texture = LoadTextureFromImage((Image) {
            .data = pixels.items,
            .width = pixels.width,
            .height = pixels.height,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
        });
    }
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "Raylib Template");
    SetTargetFPS(60);

    // Image image = LoadImage("1280px-Broadway_tower_edit.jpg");
    Image image = LoadImage("broadway-128.png");

    Texture2D texture = LoadTextureFromImage(image);
    float r = 0.3;
    float step = 0.01;
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_UP)) {
            r += step;
            cluter_image(image, r, &texture);
        }
        if (IsKeyPressed(KEY_DOWN)) {
            if (r >= step) r -= step;
            cluter_image(image, r, &texture);
        }
        if (IsKeyPressed(KEY_SPACE)) {
            cluter_image(image, r, &texture);
        }
        BeginDrawing();
        ClearBackground(GetColor(0x181818FF));
        DrawTextureEx(texture, Vector2Zero(), 0, 10, WHITE);
        DrawText(TextFormat("r = %f", r), 0, 0, 69, BLACK);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
