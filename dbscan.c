#include <raylib.h>
#include <raymath.h>

#include "nob.h"

#define POINT_RADIUS 10
#define POINT_COLOR   RED
#define VISITED_COLOR BLUE

#define DBSCAN_RADIUS 50
#define DBSCAN_MINPTS 4

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

typedef struct {
    Vector2 position;
    int label;
} Point;

typedef struct {
    Point *items;
    size_t count;
    size_t capacity;
} Points;

typedef struct {
    size_t *items;
    size_t count;
    size_t capacity;
} Indices;

void get_neighbors(const Points *points, size_t target, float radius, Indices *neighbors)
{
    if (target >= points->count) return;
    for (size_t index = 0; index < points->count; ++index) {
        if (index == target) continue;
        Vector2 p1 = points->items[index].position;
        Vector2 p2 = points->items[target].position;
        if (Vector2Distance(p1, p2) <= radius) {
            da_append(neighbors, index);
        }
    }
}

void bfs_cluster(Points *points)
{
    static Indices wave = {0};
    static Indices next_wave = {0};
    static Indices neighbors = {0};

    da_foreach(Point, point, points) {
        point->label = -1;
    }

    for (int label = 1; true; label++) {
        wave.count = 0;
        for (size_t target = 0; target < points->count; ++target) {
            if (points->items[target].label < 0) {
                neighbors.count = 0;
                get_neighbors(points, target, DBSCAN_RADIUS, &neighbors);

                if (neighbors.count + 1 < DBSCAN_MINPTS) {
                    points->items[target].label = 0;
                } else {
                    points->items[target].label = label;
                    da_append(&wave, target);
                    break;
                }
            }
        }

        if (wave.count == 0) break;

        while (wave.count > 0) {
            next_wave.count = 0;
            da_foreach(size_t, target, &wave) {
                neighbors.count = 0;
                get_neighbors(points, *target, DBSCAN_RADIUS, &neighbors);
                if (neighbors.count + 1 < DBSCAN_MINPTS) continue;
                da_foreach(size_t, nbor, &neighbors) {
                    if (points->items[*nbor].label <= 0) {
                        da_append(&next_wave, *nbor);
                        points->items[da_last(&next_wave)].label = label;
                    }
                }
            }
            swap(Indices, wave, next_wave);
        }
    }
}

int main(void)
{
    Points points = {0};

    InitWindow(800, 600, "Raylib Template");
    SetTargetFPS(60);
    bool show_radius = true;
    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Point point = {
                .position = GetMousePosition(),
                .label = -1,
            };
            da_append(&points, point);
        }
        if (IsKeyPressed(KEY_R)) {
            show_radius = !show_radius;
        }
        if (IsKeyPressed(KEY_SPACE)) {
            bfs_cluster(&points);
        }
        BeginDrawing();
        ClearBackground(GetColor(0x181818FF));
        da_foreach(Point, point, &points) {
            Color color = GRAY;
            if (point->label > 0) color = all_colors[point->label%ARRAY_LEN(all_colors)];
            DrawCircleV(point->position, POINT_RADIUS, color);
            if (show_radius) {
                DrawRing(point->position, DBSCAN_RADIUS, DBSCAN_RADIUS + 2, 0, 360, 69, color);
            }
            DrawText(TextFormat("%d", point->label), point->position.x, point->position.y, 28, WHITE);
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
