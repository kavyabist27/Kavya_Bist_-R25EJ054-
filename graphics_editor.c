

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* ─── Canvas dimensions ─────────────────────────────────────────── */
#define ROWS   30
#define COLS   60
#define BG     '.'
#define FG     '*'

/* ─── Shape types ────────────────────────────────────────────────── */
typedef enum { SHAPE_CIRCLE = 1, SHAPE_RECT, SHAPE_LINE, SHAPE_TRIANGLE } ShapeType;

/* ─── Shape data structures ──────────────────────────────────────── */
typedef struct { int cx, cy, r;              } Circle;
typedef struct { int x, y, w, h;             } Rect;
typedef struct { int x1, y1, x2, y2;         } Line;
typedef struct { int x1,y1, x2,y2, x3,y3;   } Triangle;

typedef union {
    Circle   circle;
    Rect     rect;
    Line     line;
    Triangle triangle;
} ShapeData;

typedef struct {
    int       id;
    ShapeType type;
    ShapeData data;
    int       active;
} Shape;

/* ─── Scene ──────────────────────────────────────────────────────── */
#define MAX_SHAPES 64

static char   canvas[ROWS][COLS];
static Shape  shapes[MAX_SHAPES];
static int    shape_count = 0;
static int    next_id     = 1;

/* ══════════════════════════════════════════════════════════════════
 *  Safe integer input — never spins on bad input or EOF
 * ══════════════════════════════════════════════════════════════════ */

/* Flush remainder of current input line */
static void flush_line(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* Returns 1 on success, 0 on EOF (caller should treat as quit) */
static int read_int(const char *msg, int *out) {
    for (;;) {
        printf("  %s: ", msg);
        fflush(stdout);
        int rc = scanf("%d", out);
        if (rc == EOF) {
            flush_line();
            return 0;
        }
        if (rc == 1) {
            return 1;
        }
        /* bad token — clear the line and retry */
        flush_line();
        printf("  (please enter a whole number)\n");
    }
}

/* ══════════════════════════════════════════════════════════════════
 *  Canvas helpers
 * ══════════════════════════════════════════════════════════════════ */

void canvas_clear(void) {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            canvas[r][c] = BG;
}

void canvas_plot(int r, int c) {
    if (r >= 0 && r < ROWS && c >= 0 && c < COLS)
        canvas[r][c] = FG;
}

void display(void) {
    printf("+");
    for (int c = 0; c < COLS; c++) printf("-");
    printf("+\n");

    for (int r = 0; r < ROWS; r++) {
        printf("|");
        for (int c = 0; c < COLS; c++)
            putchar(canvas[r][c]);
        printf("|\n");
    }

    printf("+");
    for (int c = 0; c < COLS; c++) printf("-");
    printf("+\n");
}

/* ══════════════════════════════════════════════════════════════════
 *  Drawing primitives
 * ══════════════════════════════════════════════════════════════════ */

void draw_line(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    for (;;) {
        canvas_plot(y1, x1);
        if (x1 == x2 && y1 == y2) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x1 += sx; }
        if (e2 <  dy) { err += dx; y1 += sy; }
    }
}

void draw_circle(int cx, int cy, int r) {
    int x = 0, y = r, d = 1 - r;
    while (x <= y) {
        canvas_plot(cy + y, cx + x); canvas_plot(cy - y, cx + x);
        canvas_plot(cy + y, cx - x); canvas_plot(cy - y, cx - x);
        canvas_plot(cy + x, cx + y); canvas_plot(cy - x, cx + y);
        canvas_plot(cy + x, cx - y); canvas_plot(cy - x, cx - y);
        if (d < 0) { d += 2 * x + 3; }
        else       { d += 2 * (x - y) + 5; y--; }
        x++;
    }
}

void draw_rect(int x, int y, int w, int h) {
    draw_line(x,     y,     x+w-1, y    );
    draw_line(x,     y+h-1, x+w-1, y+h-1);
    draw_line(x,     y,     x,     y+h-1);
    draw_line(x+w-1, y,     x+w-1, y+h-1);
}

void draw_triangle(int x1,int y1, int x2,int y2, int x3,int y3) {
    draw_line(x1,y1, x2,y2);
    draw_line(x2,y2, x3,y3);
    draw_line(x3,y3, x1,y1);
}

/* ══════════════════════════════════════════════════════════════════
 *  Render all active shapes
 * ══════════════════════════════════════════════════════════════════ */

void render_all(void) {
    canvas_clear();
    for (int i = 0; i < shape_count; i++) {
        if (!shapes[i].active) continue;
        switch (shapes[i].type) {
            case SHAPE_CIRCLE: {
                Circle *c = &shapes[i].data.circle;
                draw_circle(c->cx, c->cy, c->r);
                break;
            }
            case SHAPE_RECT: {
                Rect *r = &shapes[i].data.rect;
                draw_rect(r->x, r->y, r->w, r->h);
                break;
            }
            case SHAPE_LINE: {
                Line *l = &shapes[i].data.line;
                draw_line(l->x1, l->y1, l->x2, l->y2);
                break;
            }
            case SHAPE_TRIANGLE: {
                Triangle *t = &shapes[i].data.triangle;
                draw_triangle(t->x1,t->y1, t->x2,t->y2, t->x3,t->y3);
                break;
            }
        }
    }
}

/* ══════════════════════════════════════════════════════════════════
 *  Object management
 * ══════════════════════════════════════════════════════════════════ */

int add_circle(int cx, int cy, int r) {
    if (shape_count >= MAX_SHAPES) { printf("Scene full!\n"); return -1; }
    Shape *s = &shapes[shape_count++];
    s->id = next_id++; s->type = SHAPE_CIRCLE; s->active = 1;
    s->data.circle = (Circle){cx, cy, r};
    printf("Added circle  id=%d  (cx=%d cy=%d r=%d)\n", s->id, cx, cy, r);
    return s->id;
}

int add_rect(int x, int y, int w, int h) {
    if (shape_count >= MAX_SHAPES) { printf("Scene full!\n"); return -1; }
    Shape *s = &shapes[shape_count++];
    s->id = next_id++; s->type = SHAPE_RECT; s->active = 1;
    s->data.rect = (Rect){x, y, w, h};
    printf("Added rect    id=%d  (x=%d y=%d w=%d h=%d)\n", s->id, x, y, w, h);
    return s->id;
}

int add_line(int x1, int y1, int x2, int y2) {
    if (shape_count >= MAX_SHAPES) { printf("Scene full!\n"); return -1; }
    Shape *s = &shapes[shape_count++];
    s->id = next_id++; s->type = SHAPE_LINE; s->active = 1;
    s->data.line = (Line){x1, y1, x2, y2};
    printf("Added line    id=%d  (%d,%d)->(%d,%d)\n", s->id, x1, y1, x2, y2);
    return s->id;
}

int add_triangle(int x1,int y1, int x2,int y2, int x3,int y3) {
    if (shape_count >= MAX_SHAPES) { printf("Scene full!\n"); return -1; }
    Shape *s = &shapes[shape_count++];
    s->id = next_id++; s->type = SHAPE_TRIANGLE; s->active = 1;
    s->data.triangle = (Triangle){x1,y1, x2,y2, x3,y3};
    printf("Added triangle id=%d  (%d,%d) (%d,%d) (%d,%d)\n",
           s->id, x1,y1, x2,y2, x3,y3);
    return s->id;
}

static Shape *find_shape(int id) {
    for (int i = 0; i < shape_count; i++)
        if (shapes[i].id == id && shapes[i].active) return &shapes[i];
    return NULL;
}

int delete_shape(int id) {
    Shape *s = find_shape(id);
    if (!s) { printf("Shape id=%d not found.\n", id); return 0; }
    s->active = 0;
    printf("Deleted shape id=%d\n", id);
    return 1;
}

int modify_circle(int id, int cx, int cy, int r) {
    Shape *s = find_shape(id);
    if (!s || s->type != SHAPE_CIRCLE) { printf("Circle id=%d not found.\n", id); return 0; }
    s->data.circle = (Circle){cx, cy, r};
    printf("Modified circle id=%d\n", id);
    return 1;
}

int modify_rect(int id, int x, int y, int w, int h) {
    Shape *s = find_shape(id);
    if (!s || s->type != SHAPE_RECT) { printf("Rect id=%d not found.\n", id); return 0; }
    s->data.rect = (Rect){x, y, w, h};
    printf("Modified rect id=%d\n", id);
    return 1;
}

int modify_line(int id, int x1, int y1, int x2, int y2) {
    Shape *s = find_shape(id);
    if (!s || s->type != SHAPE_LINE) { printf("Line id=%d not found.\n", id); return 0; }
    s->data.line = (Line){x1, y1, x2, y2};
    printf("Modified line id=%d\n", id);
    return 1;
}

int modify_triangle(int id, int x1,int y1, int x2,int y2, int x3,int y3) {
    Shape *s = find_shape(id);
    if (!s || s->type != SHAPE_TRIANGLE) { printf("Triangle id=%d not found.\n", id); return 0; }
    s->data.triangle = (Triangle){x1,y1, x2,y2, x3,y3};
    printf("Modified triangle id=%d\n", id);
    return 1;
}

void list_shapes(void) {
    printf("\n-- Active shapes ---------------------\n");
    int found = 0;
    for (int i = 0; i < shape_count; i++) {
        Shape *s = &shapes[i];
        if (!s->active) continue;
        found = 1;
        switch (s->type) {
            case SHAPE_CIRCLE:
                printf("  [%d] CIRCLE   cx=%d cy=%d r=%d\n",
                    s->id, s->data.circle.cx, s->data.circle.cy, s->data.circle.r);
                break;
            case SHAPE_RECT:
                printf("  [%d] RECT     x=%d y=%d w=%d h=%d\n",
                    s->id, s->data.rect.x, s->data.rect.y,
                           s->data.rect.w, s->data.rect.h);
                break;
            case SHAPE_LINE:
                printf("  [%d] LINE     (%d,%d)->(%d,%d)\n",
                    s->id, s->data.line.x1, s->data.line.y1,
                           s->data.line.x2, s->data.line.y2);
                break;
            case SHAPE_TRIANGLE:
                printf("  [%d] TRIANGLE (%d,%d) (%d,%d) (%d,%d)\n",
                    s->id,
                    s->data.triangle.x1, s->data.triangle.y1,
                    s->data.triangle.x2, s->data.triangle.y2,
                    s->data.triangle.x3, s->data.triangle.y3);
                break;
        }
    }
    if (!found) printf("  (none)\n");
    printf("--------------------------------------\n\n");
}

/* ══════════════════════════════════════════════════════════════════
 *  Menu handlers
 * ══════════════════════════════════════════════════════════════════ */

/* Returns 0 if EOF was hit (propagate quit) */
static int menu_add(void) {
    printf("\n  1) Circle  2) Rectangle  3) Line  4) Triangle\n");
    int ch;
    if (!read_int("Choice", &ch)) return 0;
    int a=0,b=0,c=0,d=0,e=0,f=0;
    switch (ch) {
        case 1:
            if (!read_int("cx",&a)||!read_int("cy",&b)||!read_int("r",&c)) return 0;
            add_circle(a,b,c); break;
        case 2:
            if (!read_int("x",&a)||!read_int("y",&b)) return 0;
            if (!read_int("w",&c)||!read_int("h",&d)) return 0;
            add_rect(a,b,c,d); break;
        case 3:
            if (!read_int("x1",&a)||!read_int("y1",&b)) return 0;
            if (!read_int("x2",&c)||!read_int("y2",&d)) return 0;
            add_line(a,b,c,d); break;
        case 4:
            if (!read_int("x1",&a)||!read_int("y1",&b)) return 0;
            if (!read_int("x2",&c)||!read_int("y2",&d)) return 0;
            if (!read_int("x3",&e)||!read_int("y3",&f)) return 0;
            add_triangle(a,b,c,d,e,f); break;
        default: printf("  Unknown shape type.\n");
    }
    return 1;
}

static int menu_delete(void) {
    int id;
    if (!read_int("Shape id to delete", &id)) return 0;
    delete_shape(id);
    return 1;
}

static int menu_modify(void) {
    list_shapes();
    int id;
    if (!read_int("Shape id to modify", &id)) return 0;
    Shape *s = find_shape(id);
    if (!s) { printf("  Not found.\n"); return 1; }
    int a=0,b=0,c=0,d=0,e=0,f=0;
    switch (s->type) {
        case SHAPE_CIRCLE:
            if (!read_int("new cx",&a)||!read_int("new cy",&b)||!read_int("new r",&c)) return 0;
            modify_circle(id,a,b,c); break;
        case SHAPE_RECT:
            if (!read_int("new x",&a)||!read_int("new y",&b)) return 0;
            if (!read_int("new w",&c)||!read_int("new h",&d)) return 0;
            modify_rect(id,a,b,c,d); break;
        case SHAPE_LINE:
            if (!read_int("new x1",&a)||!read_int("new y1",&b)) return 0;
            if (!read_int("new x2",&c)||!read_int("new y2",&d)) return 0;
            modify_line(id,a,b,c,d); break;
        case SHAPE_TRIANGLE:
            if (!read_int("new x1",&a)||!read_int("new y1",&b)) return 0;
            if (!read_int("new x2",&c)||!read_int("new y2",&d)) return 0;
            if (!read_int("new x3",&e)||!read_int("new y3",&f)) return 0;
            modify_triangle(id,a,b,c,d,e,f); break;
    }
    return 1;
}

/* ══════════════════════════════════════════════════════════════════
 *  Demo scene
 * ══════════════════════════════════════════════════════════════════ */

void demo_scene(void) {
    printf("\n=== Demo scene ===\n");
    add_rect    (1,  1, 20, 10);
    add_circle  (45, 14,  8);
    add_line    (0,  0, 59, 29);
    add_triangle(28, 1, 15, 18, 42, 18);
    render_all();
    display();
}

/* ══════════════════════════════════════════════════════════════════
 *  main
 * ══════════════════════════════════════════════════════════════════ */

int main(void) {
    canvas_clear();
    demo_scene();

    int running = 1;
    while (running) {
        printf("\n+- MENU ----------------------------------+\n");
        printf("|  1) Add shape                           |\n");
        printf("|  2) Delete shape                        |\n");
        printf("|  3) Modify shape                        |\n");
        printf("|  4) List shapes                         |\n");
        printf("|  5) Render & display                    |\n");
        printf("|  6) Clear all shapes                    |\n");
        printf("|  0) Quit                                |\n");
        printf("+-----------------------------------------+\n");

        int ch;
        if (!read_int("Choice", &ch)) {
            /* EOF (Ctrl-D) — treat as quit */
            running = 0;
            break;
        }

        int ok = 1;
        switch (ch) {
            case 1: ok = menu_add();    if (ok) { render_all(); display(); } break;
            case 2: ok = menu_delete(); if (ok) { render_all(); display(); } break;
            case 3: ok = menu_modify(); if (ok) { render_all(); display(); } break;
            case 4: list_shapes(); break;
            case 5: render_all(); display(); break;
            case 6:
                shape_count = 0; next_id = 1;
                canvas_clear(); display();
                printf("All shapes cleared.\n"); break;
            case 0: running = 0; break;
            default: printf("Unknown option — enter 0-6.\n");
        }
        if (!ok) running = 0;   /* EOF during sub-menu => quit */
    }
    printf("Bye!\n");
    return 0;
}
