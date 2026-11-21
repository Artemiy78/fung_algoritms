#include "../utils.h"
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define IS_DIG(c) ((c) >= '0' && (c) <= '9')
#define LINE 512
#define WS " \t\r\n"
#define EPS 1e-9
#define R 6371.0

typedef enum
{
    STOP_START,
    STOP_INTERMEDIATE,
    STOP_END
} stop_type_t;

typedef struct
{
    int day, month, year, hour, minute, second;
} datetime_t;

typedef struct
{
    double lat, lon;
} coords_t;

typedef struct stop
{
    datetime_t arrival, departure;
    stop_type_t type;
    coords_t loc;
    struct stop *next;
} stop_t;

typedef struct vehicle
{
    char *id;
    stop_t *stops;
    struct vehicle *next;
} vehicle_t;

void free_stops(stop_t *s)
{
    while (s)
    {
        stop_t *next = s->next;
        free(s);
        s = next;
    }
}

void free_vehicles(vehicle_t *v)
{
    while (v)
    {
        vehicle_t *next = v->next;
        free(v->id);
        free_stops(v->stops);
        free(v);
        v = next;
    }
}

status_t dup_str(const char *src, char **out)
{
    if (!src || !out)
        return STATUS_NULL_POINTER;
    size_t len = strlen(src) + 1;
    *out = malloc(len);
    if (!*out)
        return STATUS_MEMORY_ERROR;
    memcpy(*out, src, len);
    return STATUS_OK;
}

int num(const char *s, int start, int len)
{
    int res = 0;
    for (int i = start; i < start + len; i++)
        res = res * 10 + (s[i] - '0');
    return res;
}

status_t parse_time(const char *s, datetime_t *dt)
{
    if (!s || !dt)
        return STATUS_NULL_POINTER;
    if (strlen(s) != 19)
        return STATUS_INVALID_ARGUMENT;

    for (int i = 0; i < 19; i++)
    {
        bool ok;
        if (i == 2 || i == 5)
            ok = (s[i] == '.');
        else if (i == 10)
            ok = (s[i] == ' ');
        else if (i == 13 || i == 16)
            ok = (s[i] == ':');
        else
            ok = IS_DIG((unsigned char)s[i]);

        if (!ok)
            return STATUS_INVALID_ARGUMENT;
    }

    dt->day = num(s, 0, 2);
    dt->month = num(s, 3, 2);
    dt->year = num(s, 6, 4);
    dt->hour = num(s, 11, 2);
    dt->minute = num(s, 14, 2);
    dt->second = num(s, 17, 2);

    if (dt->month < 1 || dt->month > 12 ||
        dt->hour > 23 || dt->minute > 59 || dt->second > 59)
        return STATUS_OUT_OF_RANGE;

    const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool leap = (dt->year % 4 == 0 && dt->year % 100 != 0) || dt->year % 400 == 0;
    int max_days = days[dt->month - 1] + (dt->month == 2 && leap ? 1 : 0);

    if (dt->day < 1 || dt->day > max_days)
        return STATUS_OUT_OF_RANGE;

    return STATUS_OK;
}

int cmp_time(const datetime_t *a, const datetime_t *b)
{
    if (a->year != b->year)
        return a->year - b->year;
    if (a->month != b->month)
        return a->month - b->month;
    if (a->day != b->day)
        return a->day - b->day;
    if (a->hour != b->hour)
        return a->hour - b->hour;
    if (a->minute != b->minute)
        return a->minute - b->minute;
    return a->second - b->second;
}

long long to_seconds(const datetime_t *dt)
{
    const int dpm[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    long long days = dt->year * 365LL + dt->year / 4 - dt->year / 100 + dt->year / 400;
    days += dpm[dt->month - 1] + dt->day;
    if (dt->month > 2 && ((dt->year % 4 == 0 && dt->year % 100 != 0) || dt->year % 400 == 0))
        days++;
    return days * 86400LL + dt->hour * 3600LL + dt->minute * 60LL + dt->second;
}

double distance(const coords_t *a, const coords_t *b)
{
    double lat1 = a->lat * M_PI / 180.0, lat2 = b->lat * M_PI / 180.0;
    double dlat = (b->lat - a->lat) * M_PI / 180.0;
    double dlon = (b->lon - a->lon) * M_PI / 180.0;
    double x = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlon / 2) * sin(dlon / 2);
    return 2 * R * atan2(sqrt(x), sqrt(1 - x));
}

status_t parse_stop_type(const char *s, stop_type_t *type)
{
    if (!s || !type)
        return STATUS_NULL_POINTER;
    if (strcmp(s, "start") == 0)
        *type = STOP_START;
    else if (strcmp(s, "intermediate") == 0)
        *type = STOP_INTERMEDIATE;
    else if (strcmp(s, "end") == 0)
        *type = STOP_END;
    else
        return STATUS_INVALID_ARGUMENT;
    return STATUS_OK;
}

status_t insert_stop_sorted(vehicle_t *v, stop_t *s)
{
    if (!v || !s)
        return STATUS_NULL_POINTER;

    if (!v->stops || cmp_time(&s->arrival, &v->stops->arrival) < 0)
    {
        s->next = v->stops;
        v->stops = s;
        return STATUS_OK;
    }

    stop_t *cur = v->stops;
    while (cur->next && cmp_time(&cur->next->arrival, &s->arrival) <= 0)
        cur = cur->next;

    s->next = cur->next;
    cur->next = s;
    return STATUS_OK;
}

status_t find_or_create_vehicle(vehicle_t **list, const char *id, vehicle_t **res)
{
    if (!list || !id || !res)
        return STATUS_NULL_POINTER;

    for (vehicle_t *cur = *list; cur; cur = cur->next)
        if (strcmp(cur->id, id) == 0)
        {
            *res = cur;
            return STATUS_OK;
        }

    vehicle_t *nv = malloc(sizeof(vehicle_t));
    if (!nv)
        return STATUS_MEMORY_ERROR;

    status_t st = dup_str(id, &nv->id);
    if (st != STATUS_OK)
    {
        free(nv);
        return st;
    }

    nv->stops = NULL;
    nv->next = *list;
    *list = nv;
    *res = nv;
    return STATUS_OK;
}

status_t load_file(const char *path, vehicle_t **list)
{
    if (!path || !list)
        return STATUS_NULL_POINTER;

    FILE *f = fopen(path, "r");
    if (!f)
        return STATUS_FILE_OPEN_ERROR;

    coords_t coords = {0};
    char line[LINE];

    if (fgets(line, sizeof(line), f) == NULL)
    {
        fclose(f);
        return STATUS_FILE_READ_ERROR;
    }

    char *t_lat = strtok(line, WS);
    char *t_lon = strtok(NULL, WS);

    if (!t_lat || !t_lon)
    {
        fclose(f);
        return STATUS_FILE_READ_ERROR;
    }

    status_t st = parse_double(t_lat, &coords.lat);
    if (st == STATUS_OK)
        st = parse_double(t_lon, &coords.lon);

    if (st != STATUS_OK)
    {
        fclose(f);
        return st;
    }

    while (st == STATUS_OK && fgets(line, sizeof(line), f))
    {
        if (line[strspn(line, WS)] == '\0')
            continue;

        char *tok[6];
        size_t cnt = 0;
        for (char *t = strtok(line, WS); t && cnt < 6; t = strtok(NULL, WS))
            tok[cnt++] = t;

        if (cnt != 6)
        {
            st = STATUS_INVALID_ARGUMENT;
            continue;
        }

        if (strlen(tok[0]) == 0)
        {
            st = STATUS_EMPTY_SUBSTRING;
            continue;
        }

        char arr_str[20], dep_str[20];
        snprintf(arr_str, sizeof(arr_str), "%s %s", tok[1], tok[2]);
        snprintf(dep_str, sizeof(dep_str), "%s %s", tok[3], tok[4]);

        datetime_t arr, dep;
        st = parse_time(arr_str, &arr);
        if (st == STATUS_OK)
            st = parse_time(dep_str, &dep);

        stop_type_t type;
        if (st == STATUS_OK)
            st = parse_stop_type(tok[5], &type);

        vehicle_t *v;
        if (st == STATUS_OK)
            st = find_or_create_vehicle(list, tok[0], &v);

        if (st == STATUS_OK)
        {
            stop_t *ns = malloc(sizeof(stop_t));
            if (!ns)
                st = STATUS_MEMORY_ERROR;
            else
            {
                ns->arrival = arr;
                ns->departure = dep;
                ns->type = type;
                ns->loc = coords;
                ns->next = NULL;
                st = insert_stop_sorted(v, ns);
                if (st != STATUS_OK)
                    free(ns);
            }
        }
    }

    if (st == STATUS_OK && ferror(f))
        st = STATUS_FILE_READ_ERROR;
    fclose(f);
    return st;
}

int count_routes(const vehicle_t *v)
{
    int cnt = 0;
    bool in = false;
    for (const stop_t *s = v->stops; s; s = s->next)
    {
        if (s->type == STOP_START)
            in = true;
        else if (s->type == STOP_END && in)
        {
            cnt++;
            in = false;
        }
    }
    return cnt;
}

double path_length(const vehicle_t *v)
{
    double total = 0.0;
    for (const stop_t *s = v->stops; s && s->next; s = s->next)
        total += distance(&s->loc, &s->next->loc);
    return total;
}

long long route_duration(const vehicle_t *v)
{
    if (!v->stops)
        return 0;
    const stop_t *last = v->stops;
    for (const stop_t *s = v->stops; s; s = s->next)
        last = s;
    return to_seconds(&last->departure) - to_seconds(&v->stops->arrival);
}

long long max_stop_dur(const vehicle_t *v)
{
    long long max = 0;
    for (const stop_t *s = v->stops; s; s = s->next)
    {
        long long d = to_seconds(&s->departure) - to_seconds(&s->arrival);
        if (d > max)
            max = d;
    }
    return max;
}

long long min_stop_dur(const vehicle_t *v)
{
    if (!v->stops)
        return 0;
    long long min = to_seconds(&v->stops->departure) - to_seconds(&v->stops->arrival);
    for (const stop_t *s = v->stops; s; s = s->next)
    {
        long long d = to_seconds(&s->departure) - to_seconds(&s->arrival);
        if (d < min)
            min = d;
    }
    return min;
}

long long total_downtime(const vehicle_t *v)
{
    long long total = 0;
    for (const stop_t *s = v->stops; s; s = s->next)
        total += to_seconds(&s->departure) - to_seconds(&s->arrival);
    return total;
}

const vehicle_t *find_best(const vehicle_t *list, int metric, bool max)
{
    const vehicle_t *best = NULL;
    long long best_i = max ? -1 : LLONG_MAX;
    double best_d = max ? -1.0 : DBL_MAX;

    for (const vehicle_t *v = list; v; v = v->next)
    {
        if (metric == 3 || metric == 4)
        {
            double val = path_length(v);
            if ((max && val - best_d > EPS) || (!max && best_d - val > EPS))
            {
                best_d = val;
                best = v;
            }
        }
        else
        {
            long long val = 0;
            if (metric == 1 || metric == 2)
                val = count_routes(v);
            else if (metric == 5 || metric == 6)
                val = route_duration(v);
            else if (metric == 7)
                val = max_stop_dur(v);
            else if (metric == 8)
                val = min_stop_dur(v);
            else if (metric == 9)
                val = total_downtime(v);

            if ((max && val > best_i) || (!max && val < best_i))
            {
                best_i = val;
                best = v;
            }
        }
    }
    return best;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Формат: [program] [файл1] [файл2] ...\n");
        return 1;
    }

    vehicle_t *list = NULL;
    status_t st = STATUS_OK;

    for (int i = 1; i < argc && st == STATUS_OK; i++)
        st = load_file(argv[i], &list);

    if (st != STATUS_OK)
    {
        print_error(st);
        free_vehicles(list);
        return 1;
    }

    if (!list)
    {
        printf("Нет данных\n");
        return 0;
    }

    char buf[LINE];
    bool done = false;

    while (!done)
    {
        printf("\n1-2: больше/меньше маршрутов\n"
               "3-4: длиннее/короче путь\n"
               "5-6: длиннее/короче маршрут\n"
               "7-8: дольше/короче остановка\n"
               "9: больше время простоя\n"
               "0: выход\n> ");

        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;

        buf[strcspn(buf, "\n")] = '\0';
        if (buf[0] == '\0')
            continue;

        int choice = 0;
        status_t st = parse_int(buf, &choice);

        if (st != STATUS_OK)
        {
            print_error(st);
            continue;
        }

        if (choice == 0)
        {
            done = true;
        }
        else if (choice < 1 || choice > 9)
        {
            printf("Неверный выбор\n");
        }
        else
        {
            const vehicle_t *res = find_best(list, choice, choice % 2 == 1);
            if (res)
                printf("Результат: %s\n", res->id);
            else
                printf("Не найдено\n");
        }
    }

    free_vehicles(list);
    return 0;
}